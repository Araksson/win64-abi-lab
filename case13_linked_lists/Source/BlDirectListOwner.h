#pragma once
#include <functional>
#include <type_traits>
#include <assert.h>
#include <bit>
#include <stdexcept>

template <typename _Ty, _Ty* _Ty::* NextField, _Ty** _Ty::* OwnerField, void* _Ty::* OwnerListField>
class BlDirectListOwner
{
	static_assert(std::is_trivially_copyable_v<_Ty>);
	static_assert(std::is_trivially_destructible_v<_Ty>);
	static_assert(std::is_standard_layout_v<_Ty>);

public:

	BlDirectListOwner() : pHead(nullptr), pTail(nullptr), nCount(0) {}

	~BlDirectListOwner()
	{
		if (this->nCount > 0)
		{
			auto RemoveNode_Ext = [] (_Ty* _pItem, decltype(this) pDirectListOwner)
			{
				pDirectListOwner->RemoveNode(_pItem);
				return false;
			};

			this->Iterate(RemoveNode_Ext, this);
		}

		this->pHead = nullptr;
		this->pTail = nullptr;
		this->nCount = 0;
	}

	BlDirectListOwner(const BlDirectListOwner&) = delete;
	BlDirectListOwner& operator=(const BlDirectListOwner&) = delete;

	BlDirectListOwner(BlDirectListOwner&&) = delete;
	BlDirectListOwner& operator=(BlDirectListOwner&&) = delete;

	inline signed long long GetCount() const
	{
		return this->nCount;
	}

	inline _Ty* GetHead() const
	{
		return this->pHead;
	}

	inline _Ty* GetTail() const
	{
		return this->pTail;
	}

	class Iterator
	{
		_Ty* pCurrent;
	public:
		explicit Iterator(_Ty* s) : pCurrent(s) {}
		_Ty& operator*() const
		{
			return *this->pCurrent;
		}

		_Ty* operator->() const
		{
			return this->pCurrent;
		}

		Iterator& operator++()
		{
			if (this->pCurrent)
			{
				this->pCurrent = this->pCurrent->*NextField;
			}

			return *this;
		}

		bool operator!=(const Iterator& o) const
		{
			return this->pCurrent != o.pCurrent;
		}
	};

	Iterator begin() const
	{
		return Iterator(this->pHead);
	}

	Iterator end() const
	{
		return Iterator(nullptr);
	}

	inline bool IsNodeInOtherList(const _Ty* pNode) const
	{
		assert(pNode);
		return (pNode->*OwnerListField != this);
	}

	inline bool IsNodeOwned(const _Ty* pNode) const
	{
		assert(pNode);
		return (pNode->*OwnerListField != nullptr);
	}

	inline _Ty* PushFront(_Ty* pNode)
	{
		if (!pNode)
		{
			return nullptr;
		}

		if (this->IsNodeOwned(pNode))
		{
			return nullptr;
		}

		pNode->*NextField = this->pHead;
		pNode->*OwnerField = &this->pHead;
		pNode->*OwnerListField = this;

		if (this->pHead)
		{
			this->pHead->*OwnerField = &(pNode->*NextField);
		}
		else
		{
			this->pTail = pNode;
		}

		this->pHead = pNode;
		++(this->nCount);

		return pNode;
	}

	inline _Ty* PushBack(_Ty* pNode)
	{
		if (!pNode)
		{
			return nullptr;
		}

		if (this->IsNodeOwned(pNode))
		{
			return nullptr;
		}

		pNode->*NextField = nullptr;
		if (!this->pTail)
		{
			pNode->*OwnerField = &this->pHead;
			this->pHead = pNode;
			this->pTail = pNode;
		}
		else
		{
			pNode->*OwnerField = &(this->pTail->*NextField);
			this->pTail->*NextField = pNode;
			this->pTail = pNode;
		}

		pNode->*OwnerListField = this;
		++(this->nCount);

		return pNode;
	}

	inline _Ty* InsertAfter(_Ty* pPrevious, _Ty* pNewNode)
	{
		if (!pNewNode)
		{
			return nullptr;
		}

		if (!pPrevious)
		{
			return this->PushFront(pNewNode);
		}

		if (this->IsNodeInOtherList(pPrevious))
		{
			return nullptr;
		}

		if (this->IsNodeOwned(pNewNode))
		{
			return nullptr;
		}

		_Ty* pNext = pPrevious->*NextField;

		pNewNode->*NextField = pNext;
		pNewNode->*OwnerField = &(pPrevious->*NextField);
		pPrevious->*NextField = pNewNode;

		if (pNext)
		{
			pNext->*OwnerField = &(pNewNode->*NextField);
		}
		else
		{
			this->pTail = pNewNode;
		}

		pNewNode->*OwnerListField = this;
		++(this->nCount);

		return pNewNode;
	}

	inline _Ty* InsertBefore(_Ty* pNext, _Ty* pNewNode)
	{
		if (!pNewNode)
		{
			return nullptr;
		}

		if (!pNext)
		{
			return this->PushBack(pNewNode);
		}

		if (this->IsNodeInOtherList(pNext))
		{
			return nullptr;
		}

		if (this->IsNodeOwned(pNewNode))
		{
			return nullptr;
		}

		if (pNext->*OwnerField == &this->pHead)
		{
			return this->PushFront(pNewNode);
		}

		return this->InsertAfter(this->GetNodeFromOwner(pNext->*OwnerField), pNewNode);
	}

	inline _Ty* GetPopFront()
	{
		_Ty* pNodeFront = this->GetHead();
		if (!pNodeFront)
		{
			return nullptr;
		}

		this->RemoveNode(pNodeFront);
		return pNodeFront;
	}

	inline _Ty* GetPopBack()
	{
		_Ty* pNodeTail = this->GetTail();
		if (!pNodeTail)
		{
			return nullptr;
		}

		this->RemoveNode(pNodeTail);
		return pNodeTail;
	}

	inline _Ty* RemoveNode(_Ty* pNode)
	{
		if (!pNode)
		{
			return nullptr;
		}

		if (this->IsNodeInOtherList(pNode))
		{
			return nullptr;
		}

		_Ty* pNext = pNode->*NextField;
		_Ty** pOwner = pNode->*OwnerField;
		if (!pOwner)
		{
			return nullptr;
		}

		*pOwner = pNext;
		if (pNext)
		{
			pNext->*OwnerField = pOwner;
		}
		else
		{
			if (pOwner == &this->pHead)
			{
				this->pTail = nullptr;
			}
			else
			{
				this->pTail = this->GetNodeFromOwner(pOwner);
			}
		}

		pNode->*NextField = nullptr;
		pNode->*OwnerField = nullptr;
		pNode->*OwnerListField = nullptr;

		--(this->nCount);
		return pNode;
	}

	template <typename Callable, typename... Args>
	inline bool Iterate(Callable&& fpCallBack, Args&&... hArgs)
	{
		_Ty* pNext = nullptr;
		for (_Ty* i = this->pHead; i; i = pNext)
		{
			pNext = i->*NextField;
			if (fpCallBack(i, hArgs...))
			{
				return true;
			}
		}

		return false;
	}

	inline bool Swap(_Ty* pItem1, _Ty* pItem2)
	{
		if (!pItem1 || !pItem2 || pItem1 == pItem2)
		{
			return false;
		}

		if (this->IsNodeInOtherList(pItem1))
		{
			return false;
		}

		if (this->IsNodeInOtherList(pItem2))
		{
			return false;
		}

		_Ty* pNextA = pItem1->*NextField;
		_Ty* pNextB = pItem2->*NextField;

		_Ty** ppOwnerA = pItem1->*OwnerField;
		_Ty** ppOwnerB = pItem2->*OwnerField;

		if (!ppOwnerA || !ppOwnerB)
		{
			return false;
		}

		if (pNextA == pItem2)
		{
			*ppOwnerA = pItem2;
			pItem2->*OwnerField = ppOwnerA;

			pItem1->*NextField = pNextB;

			if (pNextB)
			{
				pNextB->*OwnerField = &(pItem1->*NextField);
			}

			pItem2->*NextField = pItem1;
			pItem1->*OwnerField = &(pItem2->*NextField);
		}
		else if (pNextB == pItem1)
		{
			*ppOwnerB = pItem1;
			pItem1->*OwnerField = ppOwnerB;

			pItem2->*NextField = pNextA;

			if (pNextA)
			{
				pNextA->*OwnerField = &(pItem2->*NextField);
			}

			pItem1->*NextField = pItem2;
			pItem2->*OwnerField = &(pItem1->*NextField);
		}
		else
		{
			*ppOwnerA = pItem2;
			*ppOwnerB = pItem1;

			pItem1->*NextField = pNextB;
			pItem2->*NextField = pNextA;

			if (pNextA)
			{
				pNextA->*OwnerField = &(pItem2->*NextField);
			}

			if (pNextB)
			{
				pNextB->*OwnerField = &(pItem1->*NextField);
			}

			pItem1->*OwnerField = ppOwnerB;
			pItem2->*OwnerField = ppOwnerA;
		}

		if (this->pTail == pItem1)
		{
			this->pTail = pItem2;
		}
		else if (this->pTail == pItem2)
		{
			this->pTail = pItem1;
		}

		return true;
	}

	template <typename Callable, typename... Args>
	inline bool ReorderList(Callable&& _fpCallBack, Args&&... _tArgs)//Singlepass
	{
		bool bSucess = false;
		for (_Ty* Node = this->pHead; Node; Node = Node->*NextField)
		{
			_Ty* Next = Node->*NextField;
			if (Next && _fpCallBack(Node, Next, _tArgs...))
			{
				this->Swap(Node, Next);
				bSucess = true;
			}
		}

		return bSucess;
	}

	inline _Ty* GetNodeFromOwner(_Ty** pOwner) const
	{
		if (!pOwner)
		{
			return nullptr;
		}

		if (pOwner == &this->pHead)
		{
			return nullptr;
		}

		char* pContainerAddr = std::bit_cast<char*>(pOwner) - GetOffsetOfNextField();
		return std::bit_cast<_Ty*>(pContainerAddr);
	}

private:
	_Ty* pHead;
	_Ty* pTail;
	signed long long nCount;

	static inline ptrdiff_t GetOffsetOfNextField()//It depends on the implementations.
	{
		return reinterpret_cast<char*>(&(reinterpret_cast<_Ty*>(0)->*NextField)) - reinterpret_cast<char*>(0);
	}
};