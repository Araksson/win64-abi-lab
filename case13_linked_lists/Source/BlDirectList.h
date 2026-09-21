#pragma once
#include <functional>
#include <type_traits>
#include <assert.h>
#include <bit>
#include <stdexcept>

template <typename _Ty, _Ty* _Ty::* NextField, _Ty* _Ty::* BackField, void* _Ty::* CurrentListField>
class BlDirectList
{
	static_assert(std::is_trivially_copyable_v<_Ty>);
	static_assert(std::is_trivially_destructible_v<_Ty>);
	static_assert(std::is_standard_layout_v<_Ty>);

private:
	_Ty* pHead;
	_Ty* pTail;
	signed long long nCount;

	inline void SetCurrentListField(_Ty* _Node) const
	{
		_Node->*CurrentListField = std::bit_cast<void*>(this);
	}

	inline void RemoveCurrentListField(_Ty* _Node) const
	{
		_Node->*CurrentListField = nullptr;
	}

	inline bool TestValidateNodeInList(_Ty* _Node) const
	{
		return (!this->CheckNodeInList(_Node));
	}

	inline bool TestNONValidateNodeInList(_Ty* _Node) const
	{
		assert(_Node);
		return (_Node->*CurrentListField != nullptr);
	}

	inline void SetNodeNext(_Ty* _Node, _Ty* _Next) const
	{
		_Node->*NextField = _Next;
	}

	inline void SetNodeBack(_Ty* _Node, _Ty* _Back) const
	{
		_Node->*BackField = _Back;
	}

	inline void Init(_Ty* _Node)
	{
		this->pHead = _Node;
		this->pTail = _Node;
	}

	inline void IncrementCount()
	{
		++(this->nCount);
	}

	inline void DecrementCount()
	{
		if (this->nCount > 0)
		{
			--(this->nCount);
		}
	}

	inline void InsertBetween(_Ty* pPrev, _Ty* pNext, _Ty* pNew)
	{
		if (pPrev)
		{
			this->SetNodeNext(pPrev, pNew);
		}

		this->SetNodeBack(pNew, pPrev);

		this->SetNodeNext(pNew, pNext);
		if (pNext)
		{
			this->SetNodeBack(pNext, pNew);
		}
	}

public:

	BlDirectList() : pHead(nullptr), pTail(nullptr), nCount(0) {}

	~BlDirectList()
	{
		while (!this->IsEmpty())
		{
			this->PopFront();
		}

		this->pHead = nullptr;
		this->pTail = nullptr;
		this->nCount = 0;
	}

	inline bool CheckNodeInList(_Ty* _Node) const
	{
		assert(_Node);
		return (_Node->*CurrentListField == this);
	}

	inline _Ty* GetNextNode(_Ty* _Node)
	{
		return _Node->*NextField;
	}

	inline _Ty* GetBackNode(_Ty* _Node)
	{
		return _Node->*BackField;
	}

	inline signed long long GetCount() const
	{
		return this->nCount;
	}

	inline _Ty* GetHead()
	{
		return this->pHead;
	}

	inline _Ty* GetTail()
	{
		return this->pTail;
	}

	inline void PushBack(_Ty* _Node)
	{
		if (!_Node)
		{
			return;
		}

		if (this->TestNONValidateNodeInList(_Node))
		{
			return;
		}

		this->SetNodeNext(_Node, nullptr);
		this->SetNodeBack(_Node, nullptr);

		if (!this->pHead)
		{
			this->Init(_Node);
		}
		else
		{
			assert(this->pTail != _Node);

			this->SetNodeNext(this->pTail, _Node);
			this->SetNodeBack(_Node, this->pTail);
			this->pTail = _Node;
		}

		this->IncrementCount();
		this->SetCurrentListField(_Node);
	}

	inline void PushFront(_Ty* _Node)
	{
		if (!_Node)
		{
			return;
		}

		if (this->TestNONValidateNodeInList(_Node))
		{
			return;
		}

		this->SetNodeNext(_Node, nullptr);
		this->SetNodeBack(_Node, nullptr);

		if (!this->pHead)
		{
			this->Init(_Node);
		}
		else
		{
			assert(this->pHead != _Node);

			this->SetNodeNext(_Node, this->pHead);
			this->SetNodeBack(this->pHead, _Node);
			this->pHead = _Node;
		}

		this->IncrementCount();
		this->SetCurrentListField(_Node);
	}

	inline void MoveToFront(_Ty* Node)
	{
		if (!Node || this->pHead == Node)
		{
			return;
		}

		if (this->TestValidateNodeInList(Node))
		{
			return;
		}

		_Ty* Next = this->GetNextNode(Node);
		_Ty* Back = this->GetBackNode(Node);

		if (Back)
		{
			this->SetNodeNext(Back, Next);
		}

		if (Next)
		{
			this->SetNodeBack(Next, Back);
		}
		else
		{
			this->pTail = Back;
		}

		this->SetNodeBack(Node, nullptr);
		this->SetNodeNext(Node, this->pHead);

		if (this->pHead)
		{
			this->SetNodeBack(this->pHead, Node);
		}

		this->pHead = Node;
		if (!this->pTail)
		{
			this->pTail = Node;
		}
	}

	inline void InsertBefore(_Ty* pNext, _Ty* pNew)
	{
		if (!pNew || !pNext || pNext == pNew)
		{
			return;
		}

		if (this->TestValidateNodeInList(pNext))
		{
			return;
		}

		if (this->TestNONValidateNodeInList(pNew))
		{
			return;
		}

		this->SetNodeNext(pNew, nullptr);
		this->SetNodeBack(pNew, nullptr);

		_Ty* pPrev = this->GetBackNode(pNext);
		this->InsertBetween(pPrev, pNext, pNew);

		if (pNext == this->pHead)
		{
			this->pHead = pNew;
		}

		this->IncrementCount();
		this->SetCurrentListField(pNew);
	}

	inline void InsertAfter(_Ty* pPrev, _Ty* pNew)
	{
		if (!pNew || !pPrev || pPrev == pNew)
		{
			return;
		}

		if (this->TestValidateNodeInList(pPrev))
		{
			return;
		}

		if (this->TestNONValidateNodeInList(pNew))
		{
			return;
		}

		this->SetNodeNext(pNew, nullptr);
		this->SetNodeBack(pNew, nullptr);

		_Ty* pNext = this->GetNextNode(pPrev);
		this->InsertBetween(pPrev, pNext, pNew);

		if (pPrev == this->pTail)
		{
			this->pTail = pNew;
		}

		this->IncrementCount();
		this->SetCurrentListField(pNew);
	}

	inline bool IsEmpty()
	{
		return !this->pHead;
	}

	inline void Insert(_Ty* _pLast, _Ty* _pNewNode)
	{
		if (!_pNewNode)
		{
			return;
		}

		if (!_pLast)
		{
			this->PushBack(_pNewNode);
			return;
		}

		if (this->TestValidateNodeInList(_pLast))
		{
			return;
		}

		if (this->TestNONValidateNodeInList(_pNewNode))
		{
			return;
		}

		this->SetNodeNext(_pNewNode, nullptr);
		this->SetNodeBack(_pNewNode, nullptr);

		_Ty* pNext = this->GetNextNode(_pLast);

		this->InsertBetween(_pLast, pNext, _pNewNode);
		if (_pLast == this->pTail)
		{
			this->pTail = _pNewNode;
		}

		this->IncrementCount();
		this->SetCurrentListField(_pNewNode);
	}

	inline void PopBack()
	{
		this->RemoveNode(this->GetTail());
	}

	inline void PopFront()
	{
		this->RemoveNode(this->GetHead());
	}

	inline _Ty* GetPopBack()
	{
		_Ty* Tail = this->GetTail();
		this->RemoveNode(Tail);
		return Tail;
	}

	inline _Ty* GetPopFront()
	{
		_Ty* Head = this->GetHead();
		this->RemoveNode(Head);
		return Head;
	}

	inline void Clean()
	{
		while (this->GetHead())
		{
			this->PopFront();
		}
	}

	inline void RemoveNode(_Ty* _Node)
	{
		if (!_Node)
		{
			return;
		}

		if (this->TestValidateNodeInList(_Node))
		{
			return;
		}

		auto Next = this->GetNextNode(_Node);
		auto Back = this->GetBackNode(_Node);

		if (this->pHead == this->pTail || (!Next && !Back))
		{
			if (this->pHead != _Node)
			{
				return;
			}

			this->pHead = nullptr;
			this->pTail = nullptr;
		}
		else if (Back)
		{
			this->SetNodeNext(Back, Next);
			if (Next)
			{
				this->SetNodeBack(Next, Back);
			}
			else
			{
				this->SetNodeNext(Back, nullptr);
				this->pTail = Back;
			}
		}
		else
		{
			this->SetNodeBack(Next, nullptr);
			this->pHead = Next;
		}

		this->SetNodeBack(_Node, nullptr);
		this->SetNodeNext(_Node, nullptr);
		this->DecrementCount();
		this->RemoveCurrentListField(_Node);
	}

	class Iterator
	{
	private:
		_Ty* _Node;
	public:
		explicit Iterator(_Ty* _Start) : _Node(_Start) {}

		_Ty& operator*() const
		{
			return *(this->_Node);
		}

		_Ty* operator->() const
		{
			return this->_Node;
		}

		Iterator& operator++()
		{
			if (this->_Node)
			{
				this->_Node = this->_Node->*NextField;
			}

			return *this;
		}

		bool operator==(const Iterator& _Other) const
		{
			return this->_Node == _Other._Node;
		}

		bool operator!=(const Iterator& _Other) const
		{
			return this->_Node != _Other._Node;
		}
	};

	Iterator begin() { return Iterator(this->pHead); }
	Iterator end() { return Iterator(nullptr); }

	template <typename Callable, typename... Args>
	inline bool Iterate(Callable&& _fpCallBack, Args&&... _tArgs)
	{
		_Ty* pNext = nullptr;
		for (_Ty* Node = this->pHead; Node; Node = pNext)
		{
			pNext = this->GetNextNode(Node);
			if (_fpCallBack(Node, _tArgs...))
			{
				return true;
			}
		}

		return false;
	}

	template <typename Compare, typename... Args>
	inline bool InsertSorted(_Ty* pNew, Compare&& fpCompare, Args&&... _tArgs)
	{
		if (!pNew)
		{
			return false;
		}

		if (this->TestNONValidateNodeInList(pNew))
		{
			return false;
		}

		if (!this->pHead)
		{
			this->PushFront(pNew);
			return true;
		}

		for (_Ty* Node = this->pHead; Node; Node = this->GetNextNode(Node))
		{
			const long nInsert = fpCompare(pNew, Node, _tArgs...);
			if (nInsert == 1)
			{
				if (Node == this->pHead)
				{
					this->PushFront(pNew);
				}
				else
				{
					this->InsertBetween(this->GetBackNode(Node), Node, pNew);
					this->IncrementCount();
					this->SetCurrentListField(pNew);
				}

				return true;
			}
			else if (nInsert == 2)
			{
				return false;
			}
		}

		this->PushBack(pNew);
		return true;
	}

	inline void Swap(_Ty* NodeA, _Ty* NodeB)
	{
		if (!NodeA || !NodeB || NodeA == NodeB)
		{
			return;
		}

		if (this->TestValidateNodeInList(NodeA))
		{
			return;
		}

		if (this->TestValidateNodeInList(NodeB))
		{
			return;
		}

		auto NextA = this->GetNextNode(NodeA);
		auto BackA = this->GetBackNode(NodeA);
		auto NextB = this->GetNextNode(NodeB);
		auto BackB = this->GetBackNode(NodeB);

		if (NextA == NodeB)
		{
			if (BackA)
			{
				this->SetNodeNext(BackA, NodeB);
			}

			if (NextB)
			{
				this->SetNodeBack(NextB, NodeA);
			}

			this->SetNodeNext(NodeB, NodeA);
			this->SetNodeBack(NodeB, BackA);

			this->SetNodeNext(NodeA, NextB);
			this->SetNodeBack(NodeA, NodeB);
		}
		else if (NextB == NodeA)
		{
			if (BackB)
			{
				this->SetNodeNext(BackB, NodeA);
			}

			if (NextA)
			{
				this->SetNodeBack(NextA, NodeB);
			}

			this->SetNodeNext(NodeA, NodeB);
			this->SetNodeBack(NodeA, BackB);

			this->SetNodeNext(NodeB, NextA);
			this->SetNodeBack(NodeB, NodeA);
		}
		else
		{
			if (BackA)
			{
				this->SetNodeNext(BackA, NodeB);
			}

			if (NextA)
			{
				this->SetNodeBack(NextA, NodeB);
			}

			if (BackB)
			{
				this->SetNodeNext(BackB, NodeA);
			}

			if (NextB)
			{
				this->SetNodeBack(NextB, NodeA);
			}

			this->SetNodeNext(NodeB, NextA);
			this->SetNodeBack(NodeB, BackA);

			this->SetNodeNext(NodeA, NextB);
			this->SetNodeBack(NodeA, BackB);
		}

		if (this->pHead == NodeA)
		{
			this->pHead = NodeB;
		}
		else if (this->pHead == NodeB)
		{
			this->pHead = NodeA;
		}

		if (this->pTail == NodeA)
		{
			this->pTail = NodeB;
		}
		else if (this->pTail == NodeB)
		{
			this->pTail = NodeA;
		}
	}

	template <typename Callable, typename... Args>
	inline bool ReorderList(Callable&& _fpCallBack, Args&&... _tArgs)//Singlepass
	{
		bool bSucess = false;
		for (_Ty* Node = this->pHead; Node; Node = this->GetNextNode(Node))
		{
			_Ty* Next = this->GetNextNode(Node);
			if (Next && _fpCallBack(Node, Next, _tArgs...))
			{
				this->Swap(Node, Next);
				bSucess = true;
			}
		}

		return bSucess;
	}
};