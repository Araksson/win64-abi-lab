#pragma once
#include <functional>
#include <type_traits>
#include <assert.h>
#include <bit>
#include <stdexcept>
#include <utility>
#include "Wrappers.h"

template <typename T>
struct BlListNode
{
	void* pCurrentList;
	BlListNode<T>* pNext;
	BlListNode<T>* pBack;
	T hData;

	explicit BlListNode(const T& _Value) : hData(_Value), pNext(nullptr), pBack(nullptr), pCurrentList(nullptr) { }

	BlListNode<T>* GetBack()
	{
		return this->pBack;
	}

	BlListNode<T>* GetNext()
	{
		return this->pNext;
	}

	T& GetData()
	{
		return this->hData;
	}

	void Swap(BlListNode<T>* pNode)
	{
		std::swap(this->hData, pNode->hData);
	}
};

template <typename T>
class BlList
{
	static_assert(std::is_trivially_copyable_v<T>);
	static_assert(std::is_trivially_destructible_v<T>);
	static_assert(std::is_standard_layout_v<T>);

public:
	BlList() : pHead(nullptr), pTail(nullptr), pDequeued(nullptr), nCountOfElements(0LL) {}

	BlList(signed long long _nReserve) : pHead(nullptr), pTail(nullptr), pDequeued(nullptr), nCountOfElements(0LL)
	{
		this->Reserve(_nReserve);
	}

	~BlList()
	{
		this->Clear();
	}

	inline BlListNode<T>* SetFront(BlListNode<T>* pNode)
	{
		if (!pNode)
		{
			return nullptr;
		}

		if (this->ValidateNONListMember(pNode))
		{
			return nullptr;
		}

		pNode->pNext = nullptr;
		pNode->pBack = nullptr;
		if (!this->pHead)
		{
			this->pHead = pNode;
			this->pTail = pNode;
		}
		else
		{
			pNode->pNext = pHead;
			this->pHead->pBack = pNode;
			this->pHead = pNode;
		}

		pNode->pCurrentList = std::bit_cast<void*>(this);
		this->IncCount();
		return pNode;
	}

	inline BlListNode<T>* SetBack(BlListNode<T>* pNode)
	{
		if (!pNode)
		{
			return nullptr;
		}

		if (this->ValidateNONListMember(pNode))
		{
			return nullptr;
		}

		pNode->pNext = nullptr;
		pNode->pBack = nullptr;
		if (!this->pHead)
		{
			this->pHead = pNode;
			this->pTail = pNode;
		}
		else
		{
			this->pTail->pNext = pNode;
			pNode->pBack = this->pTail;
			this->pTail = pNode;
		}

		pNode->pCurrentList = std::bit_cast<void*>(this);
		this->IncCount();
		return pNode;
	}

	inline BlListNode<T>* PushBack(const T& hValue)
	{
		return this->SetBack(this->NewNode(hValue));
	}

	inline BlListNode<T>* PushFront(const T& hValue)
	{
		return this->SetFront(this->NewNode(hValue));
	}

	bool inline PopFront(T& hValue)
	{
		BlListNode<T>* pTmp = this->pHead;
		if (!pTmp)
		{
			return false;
		}

		if (!this->DisconectNode(pTmp))
		{
			return false;
		}

		hValue = pTmp->hData;
		this->Dequeued(pTmp);
		return true;
	}

	bool inline PopBack(T& hValue)
	{
		BlListNode<T>* pTmp = this->pTail;
		if (!pTmp)
		{
			return false;
		}

		if (!this->DisconectNode(pTmp))
		{
			return false;
		}

		hValue = pTmp->hData;
		this->Dequeued(pTmp);
		return true;
	}

	T& GetFront()
	{
		assert(this->pHead);
		return this->pHead->hData;
	}

	T& GetBack()
	{
		assert(this->pTail);
		return this->pTail->hData;
	}

	void DequeuedAll()
	{
		T hValue;
		while (this->PopFront(hValue))
		{
			//DummyPop
		}
	}

	void inline Clear()
	{
		this->DequeuedAll();
		while (this->pDequeued)
		{
			BlListNode<T>* pNextDeq = this->pDequeued->pNext;
			MEMORY_Free(nullptr, this->pDequeued);
			this->pDequeued = pNextDeq;
		}

		this->pDequeued = nullptr;
	}

	BlListNode<T>* GetFirst()
	{
		return this->pHead;
	}

	BlListNode<T>* GetTail()
	{
		return this->pTail;
	}

	BlListNode<T>* GetNext(BlListNode<T>* pNode)
	{
		if (!pNode)
		{
			return nullptr;
		}

		if (this->ValidateListMember(pNode))
		{
			return nullptr;
		}

		if (pNode == this->pTail)
		{
			return nullptr;
		}

		return pNode->pNext;
	}

	bool DisconectNode(BlListNode<T>* pNode)
	{
		if (!pNode)
		{
			return false;
		}

		if (this->ValidateListMember(pNode))
		{
			return false;
		}

		if (this->pHead == this->pTail)
		{
			this->pHead = nullptr;
			this->pTail = nullptr;
		}
		else if (pNode->pBack)
		{
			pNode->pBack->pNext = pNode->pNext;
			if (pNode->pNext)
			{
				pNode->pNext->pBack = pNode->pBack;
			}
			else
			{
				this->pTail = pNode->pBack;
				this->pTail->pNext = nullptr;
			}
		}
		else
		{
			this->pHead = pNode->pNext;
			this->pHead->pBack = nullptr;
		}

		pNode->pCurrentList = nullptr;
		pNode->pBack = nullptr;
		pNode->pNext = nullptr;
		this->DecCount();

		return true;
	}

	void RemoveNode(BlListNode<T>* pNode)
	{
		if (!this->DisconectNode(pNode))
		{
			return;
		}

		this->Dequeued(pNode);
	}

	void SetFront(const T& hItem)
	{
		for (auto h = this->GetFirst(); h; h = this->GetNext(h))
		{
			if (h->hData == hItem)
			{
				this->DisconectNode(h);
				this->SetFront(h);
				return;
			}
		}
	}

	void SetBack(const T& hItem)
	{
		for (auto h = this->GetFirst(); h; h = this->GetNext(h))
		{
			if (h->hData == hItem)
			{
				this->DisconectNode(h);
				this->SetBack(h);
				return;
			}
		}
	}

	void RemoveItem(T& hItem)
	{
		for (auto h = this->GetFirst(); h; h = this->GetNext(h))
		{
			if (h->hData == hItem)
			{
				this->RemoveNode(h);
				return;
			}
		}
	}

	bool TestItem(const T& hItem)
	{
		for (auto h = this->GetFirst(); h; h = this->GetNext(h))
		{
			if (h->hData == hItem)
			{
				return true;
			}
		}

		return false;
	}

	//Obtiene la cantidad de elementos de la lista
	signed long long inline GetNumOfElements() const
	{
		return this->nCountOfElements;
	}

	class Iterator
	{
	private:
		BlListNode<T>* itCurrent;

	public:
		explicit Iterator(BlListNode<T>* _pStart) : itCurrent(_pStart) {}

		T& operator*()
		{
			return this->itCurrent->hData;
		}

		Iterator& operator++()
		{
			if (this->itCurrent)
			{
				this->itCurrent = this->itCurrent->pNext;
			}

			return *this;
		}

		Iterator& operator--()
		{
			if (this->itCurrent)
			{
				this->itCurrent = this->itCurrent->pBack;
			}

			return *this;
		}

		bool operator!=(const Iterator& other) const
		{
			return this->itCurrent != other.itCurrent;
		}

		bool operator==(const Iterator& other) const
		{
			return this->itCurrent == other.itCurrent;
		}
	};

	Iterator begin()
	{
		return Iterator(this->pHead);
	}

	Iterator end()
	{
		return Iterator(nullptr);
	}

	T& operator[](signed long long _nIndex)
	{
		if (_nIndex >= 0 && this->GetNumOfElements() > _nIndex)
		{
			for (auto& Elem : *this)
			{
				if (_nIndex-- <= 0)
				{
					return Elem;
				}
			}
		}

		throw std::out_of_range("Index out of range");
	}

private:

	BlListNode<T>* NewNode(const T& hValue)
	{
		BlListNode<T>* pNode = nullptr;
		if (this->pDequeued)
		{
			pNode = this->pDequeued;
			this->pDequeued = this->pDequeued->pNext;
		}
		else
		{
			pNode = this->NewNode();
		}

		memcpy(&pNode->hData, (void*)(&hValue), sizeof(T));
		pNode->pNext = nullptr;
		pNode->pBack = nullptr;
		pNode->pCurrentList = nullptr;
		return pNode;
	}

	void Reserve(signed long long nElements)
	{
		while (nElements-- > 0)
		{
			this->Dequeued(this->NewNode());
		}
	}

	void inline Dequeued(BlListNode<T>* pNode)
	{
		pNode->pCurrentList = nullptr;
		pNode->pBack = nullptr;
		pNode->pNext = this->pDequeued;
		this->pDequeued = pNode;
	}

	BlListNode<T>* NewNode()
	{
		return std::bit_cast<BlListNode<T>*>(MEMORY_Alloc(nullptr, sizeof(BlListNode<T>)));
	}

	inline void IncCount()
	{
		++(this->nCountOfElements);
	}

	inline void DecCount()
	{
		--(this->nCountOfElements);
	}

	inline bool CheckNodeInCurrentList(BlListNode<T>* pNode) const
	{
		assert(pNode);
		return (pNode->pCurrentList == this);
	}

	inline bool ValidateListMember(BlListNode<T>* pNode) const
	{
		return (!this->CheckNodeInCurrentList(pNode));
	}

	inline bool ValidateNONListMember(BlListNode<T>* pNode) const
	{
		assert(pNode);
		return (pNode->pCurrentList != nullptr);
	}

	BlListNode<T>* pHead;
	BlListNode<T>* pTail;
	BlListNode<T>* pDequeued;
	signed long long nCountOfElements;
};