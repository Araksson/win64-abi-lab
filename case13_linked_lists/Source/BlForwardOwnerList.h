#pragma once
#include <functional>
#include <type_traits>
#include <assert.h>
#include <bit>
#include <stdexcept>
#include "Wrappers.h"

template <typename T>
struct BlFONode
{
	void* pCurrentList;
	BlFONode<T>* pNext;
	BlFONode<T>** pOwner;
	T hData;

	BlFONode() : pNext(nullptr), pOwner(nullptr), pCurrentList(nullptr) { }
	BlFONode(const T& v) : pNext(nullptr), pOwner(nullptr), pCurrentList(nullptr), hData(v) { }

	BlFONode<T>* GetNext()
	{
		return this->pNext;
	}

	T& GetData()
	{
		return this->hData;
	}
};

template <typename T>
class BlForwardOwnerList
{
	static_assert(std::is_trivially_copyable_v<T>);
	static_assert(std::is_trivially_destructible_v<T>);
	static_assert(std::is_standard_layout_v<T>);

public:
	BlForwardOwnerList() : pHead(nullptr), pTailOwner(&pHead), pDequeued(nullptr), nCountOfElements(0) {}

	explicit BlForwardOwnerList(unsigned long long _nReserve) : BlForwardOwnerList()
	{
		this->Reserve(_nReserve);
	}

	~BlForwardOwnerList()
	{
		this->Clear();
	}

	BlForwardOwnerList(const BlForwardOwnerList&) = delete;
	BlForwardOwnerList& operator=(const BlForwardOwnerList&) = delete;

	BlForwardOwnerList(BlForwardOwnerList&&) = delete;
	BlForwardOwnerList& operator=(BlForwardOwnerList&&) = delete;

	inline void PushFront(const T& hValue)
	{
		this->InsertAtHead(this->NewNode(hValue), true);
	}

	inline void PushBack(const T& hValue)
	{
		BlFONode<T>* pNode = this->NewNode(hValue);

		pNode->pCurrentList = std::bit_cast<void*>(this);
		pNode->pNext = nullptr;
		pNode->pOwner = this->pTailOwner;
		*(this->pTailOwner) = pNode;

		this->pTailOwner = &pNode->pNext;

		++(this->nCountOfElements);
	}

	bool PopFront(T& hOut)
	{
		if (!this->pHead)
		{
			return false;
		}

		BlFONode<T>* pTmp = this->pHead;
		hOut = this->pHead->hData;

		*(pTmp->pOwner) = pTmp->pNext;
		if (pTmp->pNext)
		{
			pTmp->pNext->pOwner = pTmp->pOwner;
		}
		else
		{
			this->pTailOwner = pTmp->pOwner;
		}

		this->Dequeued(pTmp);
		--(this->nCountOfElements);
		return true;
	}

	bool PopBack(T& hOut)
	{
		if (!this->pHead)
		{
			return false;
		}

		if (!this->pHead->pNext)
		{
			return this->PopFront(hOut);
		}

		BlFONode<T>* pPrevNode = this->pHead;
		while (pPrevNode->pNext && pPrevNode->pNext->pNext)
		{
			pPrevNode = pPrevNode->pNext;
		}

		BlFONode<T>* pLastNode = pPrevNode->pNext;
		hOut = pLastNode->hData;

		pPrevNode->pNext = nullptr;
		this->pTailOwner = &pPrevNode->pNext;

		this->Dequeued(pLastNode);
		--(this->nCountOfElements);
		return true;
	}

	BlFONode<T>* GetFirst()
	{
		return this->pHead;
	}

	BlFONode<T>* GetNext(BlFONode<T>* pNode)
	{
		return pNode ? pNode->pNext : nullptr;
	}

	bool TestItem(const T& hItem) const
	{
		for (auto n = this->pHead; n; n = n->pNext)
		{
			if (n->hData == hItem)
			{
				return true;
			}
		}

		return false;
	}

	inline void RemoveNode(BlFONode<T>* pNode)
	{
		if (!pNode)
		{
			return;
		}

		if (pNode->pCurrentList != this || !pNode->pOwner)
		{
			return;
		}

		*(pNode->pOwner) = pNode->pNext;
		if (pNode->pNext)
		{
			pNode->pNext->pOwner = pNode->pOwner;
		}
		else
		{
			this->pTailOwner = pNode->pOwner;
		}

		this->Dequeued(pNode);
		--(this->nCountOfElements);
	}

	void RemoveItem(const T& item)
	{
		for (auto n = this->pHead; n; n = n->pNext)
		{
			if (n->hData == item)
			{
				this->RemoveNode(n);
				return;
			}
		}
	}

	void SetFront(const T& iItem)
	{
		BlFONode<T>* pNextIt = nullptr;
		for (auto n = this->pHead; n; n = pNextIt)
		{
			pNextIt = n->pNext;
			if (n->hData == iItem)
			{
				*(n->pOwner) = pNextIt;
				if (pNextIt)
				{
					pNextIt->pOwner = n->pOwner;
				}
				else
				{
					this->pTailOwner = n->pOwner;
				}

				this->InsertAtHead(n, false);
				break;
			}
		}
	}

	void SetBack(const T& iItem)
	{
		BlFONode<T>* pNextIt = nullptr;
		for (auto n = this->pHead; n; n = pNextIt)
		{
			pNextIt = n->pNext;
			if (n->hData == iItem)
			{
				*(n->pOwner) = pNextIt;
				if (pNextIt)
				{
					pNextIt->pOwner = n->pOwner;
				}
				else
				{
					this->pTailOwner = n->pOwner;
				}

				n->pNext = nullptr;
				n->pOwner = this->pTailOwner;
				*(this->pTailOwner) = n;
				this->pTailOwner = &n->pNext;
				break;
			}
		}
	}

	void Clear()
	{
		while (this->pHead)
		{
			BlFONode<T>* pCurrent = this->pHead;
			this->pHead = pCurrent->pNext;

			if (this->pHead)
			{
				this->pHead->pOwner = &this->pHead;
			}
			else
			{
				this->pTailOwner = &this->pHead;
			}

			this->Dequeued(pCurrent);
		}

		while (this->pDequeued)
		{
			BlFONode<T>* pNextNode = this->pDequeued->pNext;
			MEMORY_Free(nullptr, this->pDequeued);
			this->pDequeued = pNextNode;
		}

		this->pTailOwner = &this->pHead;
		this->nCountOfElements = 0;
	}

	void Reserve(unsigned long long nElements)
	{
		while (nElements-- > 0)
		{
			this->Dequeued(this->NewNode());
		}
	}

	unsigned long long GetNumOfElements() const { return this->nCountOfElements; }

	class Iterator
	{
		BlFONode<T>* pCurr;
	public:
		Iterator(BlFONode<T>* pStart) : pCurr(pStart) {}

		T& operator*()
		{
			return this->pCurr->hData;
		}

		Iterator& operator++()
		{
			if (this->pCurr)
			{
				this->pCurr = this->pCurr->pNext;
			}

			return *this;
		}

		bool operator!=(const Iterator& o) const
		{
			return this->pCurr != o.pCurr;
		}

		bool operator==(const Iterator& o) const
		{
			return this->pCurr == o.pCurr;
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

private:

	inline void InsertAtHead(BlFONode<T>* pNode, bool bIncrementCount)
	{
		BlFONode<T>* pFirstElement = this->pHead;

		pNode->pCurrentList = std::bit_cast<void*>(this);
		pNode->pNext = pFirstElement;
		pNode->pOwner = &this->pHead;
		if (pFirstElement)
		{
			pFirstElement->pOwner = &pNode->pNext;
		}
		else
		{
			this->pTailOwner = &pNode->pNext;
		}

		this->pHead = pNode;
		if (bIncrementCount)
		{
			++(this->nCountOfElements);
		}
	}

	BlFONode<T>* NewNode()
	{
		BlFONode<T>* pNode = nullptr;
		if (this->pDequeued)
		{
			pNode = this->pDequeued;
			this->pDequeued = this->pDequeued->pNext;
		}
		else
		{
			pNode = std::bit_cast<BlFONode<T>*>(MEMORY_Alloc(nullptr, sizeof(BlFONode<T>)));
		}

		memset(pNode, 0x0, sizeof(BlFONode<T>));
		return pNode;
	}

	BlFONode<T>* NewNode(const T& hValue)
	{
		BlFONode<T>* pNode = this->NewNode();
		memcpy(&pNode->hData, (void*)(&hValue), sizeof(T));
		return pNode;
	}

	inline void Dequeued(BlFONode<T>* pNode)
	{
		pNode->pCurrentList = nullptr;
		pNode->pOwner = nullptr;
		pNode->pNext = this->pDequeued;
		this->pDequeued = pNode;
	}

private:
	BlFONode<T>* pHead;
	BlFONode<T>** pTailOwner;
	BlFONode<T>* pDequeued;
	signed long long nCountOfElements;
};