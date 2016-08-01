#ifndef MAP_HPP_
#define MAP_HPP_
#include <iostream>
#include <algorithm>
#include <functional>
#include <cmath>
#include <stack>
#include <exception>
#include <cassert>
namespace cs540{
	template <class T1, class T2, class comp = std::less<T1> >
	class Map
	{
		typedef  const typename  std::remove_cv< typename std::remove_reference<T1>::type >::type keyType;
		typedef typename std::remove_reference<T2>::type objType;
		using value_type = std::pair<keyType, objType>;
		template <class T> friend class Generic_Iterator;
		friend class ReverseIterator;
		/////////////////nested class declarations	
		struct tree_node_t;
		////iterators/////		
	private:
		template <class T>
		class Generic_Iterator
		{
			typedef typename std::remove_reference <T>::type retType;
			friend class Map;
			tree_node_t * m_pcur;
			const Map * m_pcurMap;
		protected:
			tree_node_t * getptr()const{ return m_pcur; }
			Generic_Iterator(tree_node_t * pnode,const Map * pmap) :m_pcur(pnode),m_pcurMap(pmap){}
			const Map* getmap()const{ return m_pcurMap; }
		public:
			inline Generic_Iterator& operator ++();
			inline Generic_Iterator operator++(int);
			inline Generic_Iterator& operator --();
			inline Generic_Iterator operator--(int);
			inline retType & operator *()const;
			template <class TT>
			bool operator==(const Generic_Iterator<TT> & it)const
			{
				return it.getptr() == this->getptr();
			}
			template <class TT>
			bool operator!=(const Generic_Iterator<TT> &it)const
			{
				return this->getptr() != it.getptr();
			}
		};

	public:
		using Iterator = Generic_Iterator<value_type >;
		using ConstIterator = Generic_Iterator<const value_type>;
		class ReverseIterator final :private Iterator
		{
			friend class Map;
		private:
			ReverseIterator(tree_node_t * pnode,const Map * pmap) :Iterator(pnode,pmap){}
		public:
			ReverseIterator& operator ++(){ 
				Iterator::operator--(); return *this; 
			}
			ReverseIterator operator++(int){ return ReverseIterator(Iterator::operator--(0).getptr(),this->getmap()); }
			ReverseIterator& operator --(){
				if (this->getptr() == nullptr) this->m_pcur = this->getmap()->m_pfirst;
				else
				Iterator::operator++(); 
				return *this; 
			}
			ReverseIterator operator--(int){ 
				auto tmp = this->getptr();
				--*this;
				return ReverseIterator(tmp,this->getmap()); 
			}
			bool operator==(const ReverseIterator & it)const
			{
				return it.getptr() == this->getptr();
			}
			bool operator!=(const ReverseIterator  &it)const
			{
				return this->getptr() != it.getptr();
			}
			value_type &operator *()
			{
				return this->getptr()->v;
			}
		};

		/////////Iterator definitions end////
	private:
		///////////////////tree nodes /////////////
		struct tree_node_t
		{
			tree_node_t *pleft, *pright, *pparent, *pprev, *pnext;
			int height;
			std::pair<keyType, objType> v;
			//constructors
			tree_node_t() = delete;
			tree_node_t(const value_type & _v, int _height = 0, tree_node_t * _pleft = nullptr,
				tree_node_t *_pright = nullptr, tree_node_t * _pparent = nullptr) :pleft(_pleft), pright(_pright),
				pparent(_pparent), pprev(nullptr), pnext(nullptr),height(_height),v(_v)
			{
			}
			tree_node_t(value_type && _v, int _height = 0, tree_node_t * _pleft = nullptr,
				tree_node_t *_pright = nullptr, tree_node_t * _pparent = nullptr) :pleft(_pleft), pright(_pright),
				pparent(_pparent),pprev(nullptr),pnext(nullptr), height(_height), v(std::forward<value_type>(_v))
			{
			}

			static  tree_node_t * skew(tree_node_t * proot)
			{
				if (proot == nullptr) 
				{
					return nullptr;
				}
				if (proot->pleft == nullptr || proot->pleft->height != proot->height)
				{

					return proot;
				}
				auto tmp = proot->pleft;
				proot->pleft = tmp->pright;
				tmp->pright = proot;
				if (proot->pleft)
				{
					proot->pleft->pparent = proot;
				}
				tmp->pparent = proot->pparent;
				proot->pparent = tmp;
				return tmp;

			}
			static  tree_node_t * split(tree_node_t * proot)
			{
				if (proot == nullptr || proot->pright == nullptr || proot->pright->pright == nullptr ||
					proot->height != proot->pright->height || proot->height != proot->pright->pright->height)
				{
					return proot;
				}
				tree_node_t * tmp = proot->pright;
				proot->pright = tmp->pleft;
				if (tmp->pleft)
				{
					tmp->pleft->pparent = proot; 
				}
				tmp->pleft = proot;
				tmp->pparent = proot->pparent;
				proot->pparent = tmp;
				tmp->height++;
				return tmp;
			}
			///////////////////////deallocation//////////
			void remove() //remove the subtree rooted at this
			{
				assert(this != nullptr);
				std::stack<tree_node_t *> st;
				st.push(this);
				/////perform pre-traversal and free nodes/////
				////this pointer will never be accessible in this process
				while (!st.empty())
				{
					auto tmp = st.top();
					st.pop();
					if (tmp->pright != nullptr) st.push(tmp->pright);
					if (tmp->pleft != nullptr) st.push(tmp->pleft);
					delete tmp;
				}

			}
			//////////////////////////static functions for balancing the tree /////////
			static tree_node_t * adjust_up_by_split(tree_node_t*pstart, const keyType & k)
			{
				comp cmp;
				if (pstart == nullptr)
				{
					return nullptr;
				}
				do
				{
					pstart = skew(pstart);
					pstart = split(pstart);
					if (pstart->pparent)
					{
						cmp(k, pstart->pparent->v.first) ? (pstart->pparent->pleft = pstart)
							: (pstart->pparent->pright = pstart);
						pstart = pstart->pparent;
					}
					else break;
				} while (1);
				return pstart;
			}
			static tree_node_t *adjust_up_by_join(tree_node_t*pstart)
			{
				if (pstart == nullptr)
					return pstart;
				do
				{
					auto tmp = pstart;
					if ((pstart->pleft && pstart->height - pstart->pleft->height > 1) ||
						(pstart->pright && pstart->height - pstart->pright->height > 1)||
						(pstart->height==1 && !(pstart->pleft && pstart->pright)))
					{
						
						if (pstart->pright && pstart->pright->height == pstart->height) 
							pstart->pright->height--;
						pstart->height--;
						pstart = skew(pstart);
						if (pstart->pright) 
							pstart->pright = skew(pstart->pright);
						if (pstart->pright &&pstart->pright->pright) 
							pstart->pright->pright = skew(pstart->pright->pright);
						pstart = split(pstart);
						pstart->pright = split(pstart->pright);
					}
					if (pstart->pparent)
					{
						pstart->pparent->pleft==tmp ? (pstart->pparent->pleft = pstart) : (pstart->pparent->pright = pstart);
						pstart = pstart->pparent;
					}
					else break;
				} while (1);
				return pstart;
			}
			static void duplicate(tree_node_t * from, tree_node_t * &to, tree_node_t *& pleft, tree_node_t *&pright)
			{
				pleft = pright = nullptr;
				if (from == nullptr) to = nullptr;
				else
				{
					tree_node_t *l1, *r1;
					tree_node_t *l2, *r2;
					to = new tree_node_t(from->v, from->height);
					duplicate(from->pleft, to->pleft, l1, r1);
					duplicate(from->pright, to->pright, l2, r2);
					if (to->pleft) to->pleft->pparent = to;
					if (to->pright) to->pright->pparent = to;
					if (r1) r1->pnext = to;
					if (l2) l2->pprev = to;
					to->pprev = r1;
					to->pnext = l2;
					if (l1)pleft = l1;
					else pleft = to;
					if (r2) pright = r2;
					else pright = to;
				}
			}


		};//end of tree node definition
	private:
		tree_node_t * m_proot, *m_pfirst, *m_plast;
		std::size_t m_size;
	public://///////////export operations//////////////
		//Constructors:
		Map() throw() :m_proot(nullptr), m_pfirst(nullptr), m_plast(nullptr), m_size(0){}
		Map(const Map & map) throw() :Map()
		{
			tree_node_t::duplicate(map.m_proot, m_proot, m_pfirst, m_plast);
			m_size = map.m_size;
		}
		Map(Map && map) throw() :m_proot(map.m_proot), m_pfirst(map.m_pfirst), m_plast(map.m_plast), m_size(map.m_size)
		{
			map.m_proot = map.m_pfirst = map.m_plast = nullptr;
			map.m_size = 0;
		}
		Map(std::initializer_list<value_type> li)throw() :Map()
		{
			for (auto & i : li)
			{
				insert(i);
			}
		}
		Map& operator=(const Map&map)throw()
		{
			if (&map != this)
			{
				this->~Map();
				new (this) Map(map);

			}
			return *this;
		}
		Map& operator=(Map&&map)throw()
		{
			if (&map != this)
			{
				this->~Map();
				new (this) Map(std::forward<Map>(map));
			}
			return *this;
		}
		~Map()throw()
		{
			clear();
		}
		//Modifiers
		void clear()throw()
		{
			if (!empty())
			{
				m_proot->remove();
				m_proot = m_pfirst = m_plast = nullptr;
				m_size = 0;
			}
		}
		Iterator insert(const value_type& _v)throw()
		{
			return insert(value_type(_v.first, _v.second));
		}
		Iterator insert(value_type&&_v)throw()
		{
			auto tmp = m_proot;
			decltype(tmp) prev = nullptr;
			if (m_proot == nullptr)
			{
				m_proot = m_pfirst = m_plast = new tree_node_t(std::forward<value_type>(_v));
				m_size = 1; 
				return Iterator(m_proot,this);
			}
			comp cmp;
			while (tmp)
			{
				prev = tmp;
				if (cmp(_v.first, tmp->v.first)){
					tmp = tmp->pleft; 
				}
				else if (cmp(tmp->v.first, _v.first)){
					tmp = tmp->pright;
				}
				else break;
			}
		
			if (tmp)
			{
				return Iterator(tmp,this);
			}
			else
			{
				
				tmp = new tree_node_t(std::forward<value_type>(_v), 0, nullptr, nullptr, prev);
				assert(prev);
				if (cmp(_v.first, prev->v.first))
				{ 
					tmp->pprev = prev->pprev;
					tmp->pnext = prev;
					prev->pprev = tmp;
					prev->pleft = tmp;
					if (tmp->pprev) tmp->pprev->pnext = tmp;
				}
				else
				{
					tmp->pprev = prev;
					tmp->pnext = prev->pnext;
					prev->pnext = tmp;
					prev->pright = tmp;
					if (tmp->pnext) {
						tmp->pnext->pprev = tmp; 
					}
				}
				if (tmp->pprev == nullptr) m_pfirst = tmp;
				if (tmp->pnext == nullptr) m_plast = tmp;
				m_size++;
				m_proot = tree_node_t::adjust_up_by_split(prev, _v.first);
			}
			return Iterator(tmp,this);
		}
		void remove(const keyType&k)throw(std::out_of_range)
		{
			auto it = find(k);
			if (it.getptr() == nullptr) 
				throw std::out_of_range("key doesnt exist");
			erase(it);
		}
		void erase(Iterator it)throw()
		{
			auto tmp = it.getptr();
			if (tmp == nullptr) return;
			m_size--;
			if (tmp->pright&&tmp->pleft)
			{
				auto p = tmp->pright;
				while (p->pleft) p = p->pleft;
				if (tmp->pparent)
				{
					std::swap(tmp->pparent->pleft == tmp ? tmp->pparent->pleft : tmp->pparent->pright, p->pparent->pleft == p ?
						p->pparent->pleft : p->pparent->pright);
				}
				else
				{
					m_proot = p;
					p->pparent->pleft == p ? (p->pparent->pleft = tmp) : (p->pparent->pright = tmp);
				}
				std::swap(tmp->pleft, p->pleft);
				std::swap(tmp->height, p->height);
				std::swap(tmp->pright, p->pright);
				std::swap(tmp->pparent, p->pparent);
				if (p->pleft)
					p->pleft->pparent = p;
				if (p->pright)
					p->pright->pparent = p;
			}
			auto p = tmp->pleft ? tmp->pleft : tmp->pright;
			if (tmp->pparent)
			{
				if (tmp->pparent->pleft == tmp)
					tmp->pparent->pleft = p;
				else
					tmp->pparent->pright = p;
			}
			else
				m_proot = p;
			if (p)
				p->pparent = tmp->pparent;
			if (tmp->pprev) 
				tmp->pprev->pnext = tmp->pnext;
			if (tmp->pnext)
				tmp->pnext->pprev = tmp->pprev;
			if (tmp == m_pfirst) 
				m_pfirst = tmp->pnext;
			if (tmp == m_plast)
				m_plast = tmp->pprev;
			if (tmp->pparent)
				m_proot = tree_node_t::adjust_up_by_join(tmp->pparent);
			delete tmp;

		}
		////look up
		Iterator find(const keyType& k)noexcept
		{
			return Iterator(const_cast<const Map *>(this)->find(k).getptr(),this);
		}
		ConstIterator find(const keyType& k) const noexcept
		{
			auto tmp = m_proot;
			comp cmp;
			while (tmp)
			{
				if (cmp(k, tmp->v.first))
					tmp = tmp->pleft;
				else if (cmp(tmp->v.first, k))
					tmp = tmp->pright;
				else
					break;
			}
			return ConstIterator(tmp,this);
		}
		objType& at(const keyType&k) throw (std::out_of_range)
		{
			auto ptr = find(k).getptr();
			if (ptr == nullptr) throw std::out_of_range("key does not exist in the map");
			return ptr->v.second;
		}
		const objType& at(const keyType&k)const throw (std::out_of_range)
		{
			auto ptr = find(k).getptr();
			if (ptr == nullptr) throw std::out_of_range("key does not exist in the map");
			return ptr->v.second;
		}
		objType& operator[](const keyType&k)throw()
		{
			try
			{
				return at(k);
			}
			catch (std::out_of_range c)
			{
				return insert(value_type(k, objType())).getptr()->v.second;
			}
		}
		////Size
		std::size_t size()const throw()
		{
			return m_size;
		}
		bool empty() const throw()
		{
			return m_proot == nullptr;
		}
		//////Comparison Operators
		bool operator==(const Map& map) const throw()
		{
			auto it = begin();
			auto it1 = map.begin();
			auto ed = end();
			auto ed1 = map.end();
			while (it != ed && it1 != ed1 &&*it == *it1)
			{
				it++;
				it1++;
			}
			return it == ed &&it1 == ed1;
		}
		bool operator!=(const Map& map) const
		{
			return !(*this == map);
		}
		//Iterators
		Iterator begin()
		{
			return Iterator(m_pfirst,this);
		}
		Iterator end()
		{
			return Iterator(nullptr,this);
		}
		ConstIterator begin() const
		{
			return ConstIterator(m_pfirst,this);
		}
		ConstIterator end()const
		{
			return ConstIterator(nullptr,this);
		}
		ReverseIterator rbegin()
		{
			return ReverseIterator(m_plast,this);
		}
		ReverseIterator rend()
		{
			return ReverseIterator(nullptr,this);
		}
		///////////for test///////
		/*int getheight()const
		{
			if (m_proot == nullptr) return -1;
			return m_proot->height;
		}
		void check()const
		{
				((Map*)this)->check();
		}
		void check()
		{
			Map::check_tree(m_proot);
			int i = 0;
			for (auto &t : *this) i++;
			assert(i == m_size);
			i = 0;
			for (auto it = end(); it != begin(); --it) i++;
			assert(i == m_size);
			i = 0;
			for (auto it = rbegin(); it != rend(); ++it) i++;
			assert(i == m_size);
			i = 0;
			for (auto it = rend(); it != rbegin(); --it) i++;
			assert(i == m_size);
			auto p = (const Map*)this;
			i = 0;
			for (auto &t : *p) i++;
			assert(i == m_size);
			i = 0;
			for (auto it =p-> end(); it != p->begin(); --it) i++;
			assert(i == m_size);
			auto k = m_pfirst;
			while (k)
			{
				if (k->pprev)
				{
					assert(comp()(k->pprev->v.first, k->v.first));
					assert(k == k->pprev->pnext);
				}
				k = k->pnext;
			}
			k = m_plast;
			while (k)
			{
				if (k->pprev)
				{
					assert(comp()(k->pprev->v.first, k->v.first));
					assert(k == k->pprev->pnext);
				}
				k = k->pprev;
			}
			auto it = begin();
			auto ttt = it;
			auto t = it++;
			assert(ttt==t);
			

		}
		static void check_tree(tree_node_t *p)///to ensure the structure of the tree is correct
		{
			if (p)
			{
				check_tree(p->pleft);
				check_tree(p->pright);
				assert(p->pleft == nullptr || p->pleft &&p->pleft->pparent == p);
				assert(p->pright==nullptr ||p->pright&&p->pright->pparent == p);
				assert(p->pleft == nullptr&&p->height==0 || p->pleft->height < p->height &&p->height-p->pleft->height<2);
				assert(p->pright == nullptr&&p->height == 0 || p->pright->height <= p->height&&p->height - p->pright->height<2);
				assert(p->pright == nullptr || p->pright->pright == nullptr || p->pright->pright->height != p->height);
			}
		}*/

	};
	template <class T1, class T2, class comp >
	template <class T>
	typename Map<T1, T2, comp>::template Generic_Iterator<T> &  Map<T1, T2, comp>::Generic_Iterator<T>::operator++()//
	{
		assert(this->m_pcur);
		this->m_pcur = m_pcur->pnext;
		return *this;
	}
	template <class T1, class T2, class comp >
	template <class T>
	typename Map<T1, T2, comp>::template Generic_Iterator<T>  Map<T1, T2, comp>::Generic_Iterator<T>::operator++(int)
	{
		assert(this->m_pcur);
		auto p = m_pcur;
		this->m_pcur = m_pcur->pnext;
		return Generic_Iterator(p,this->getmap());
	}
	template <class T1, class T2, class comp >
	template <class T>
	typename Map<T1, T2, comp>::template Generic_Iterator<T>& Map<T1, T2, comp>::Generic_Iterator<T>::operator--()
	{
		if (this->m_pcur)
		m_pcur = m_pcur->pprev;
		else m_pcur = this->getmap()->m_plast;
		return *this;
	}
	template <class T1, class T2, class comp >
	template <class T>
	typename Map<T1, T2, comp>::template Generic_Iterator<T> Map<T1, T2, comp>::Generic_Iterator<T>::operator--(int)
	{
		auto p = m_pcur;
		--*this;
		return Generic_Iterator(p,this->getmap());
	}
	template <class T1, class T2, class comp >
	template <class T>
	typename Map<T1, T2, comp>::template Generic_Iterator<T>::retType &   Map<T1, T2, comp>::Generic_Iterator<T>::operator*()const
	{
		return (m_pcur->v);
	}
}
#endif
