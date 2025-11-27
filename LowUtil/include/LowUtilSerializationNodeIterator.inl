
// ---------- Iteration support ----------
struct Item
{
  const String *key; // nullptr for sequences
  Node &value;
};
struct ConstItem
{
  const String *key; // nullptr for sequences
  const Node &value;
};

struct iterator
{
  // sequence state
  Seq *s = nullptr;
  std::size_t i = 0;

  // dict state
  Dict *m = nullptr;
  typename Dict::iterator mi{};

  using difference_type = std::ptrdiff_t;
  using value_type = Item;
  using reference = Item;
  using iterator_category = std::forward_iterator_tag;

  iterator() = default;

  static iterator seq_begin(Seq *sp)
  {
    iterator it;
    it.s = sp;
    it.i = 0;
    return it;
  }
  static iterator seq_end(Seq *sp)
  {
    iterator it;
    it.s = sp;
    it.i = sp ? sp->size() : 0;
    return it;
  }
  static iterator map_begin(Dict *mp)
  {
    iterator it;
    it.m = mp;
    it.mi = mp ? mp->begin() : typename Dict::iterator{};
    return it;
  }
  static iterator map_end(Dict *mp)
  {
    iterator it;
    it.m = mp;
    it.mi = mp ? mp->end() : typename Dict::iterator{};
    return it;
  }

  reference operator*() const
  {
    if (s)
      return Item{nullptr, (*s)[i]};
    return Item{&mi->first, mi->second};
  }

  iterator &operator++()
  {
    if (s)
      ++i;
    else
      ++mi;
    return *this;
  }
  bool operator==(const iterator &o) const
  {
    if (s || o.s)
      return s == o.s && i == o.i;
    return m == o.m && mi == o.mi;
  }
  bool operator!=(const iterator &o) const
  {
    return !(*this == o);
  }
};

struct const_iterator
{
  const Seq *s = nullptr;
  std::size_t i = 0;

  const Dict *m = nullptr;
  typename Dict::const_iterator mi{};

  using difference_type = std::ptrdiff_t;
  using value_type = ConstItem;
  using reference = ConstItem;
  using iterator_category = std::forward_iterator_tag;

  const_iterator() = default;

  static const_iterator seq_begin(const Seq *sp)
  {
    const_iterator it;
    it.s = sp;
    it.i = 0;
    return it;
  }
  static const_iterator seq_end(const Seq *sp)
  {
    const_iterator it;
    it.s = sp;
    it.i = sp ? sp->size() : 0;
    return it;
  }
  static const_iterator map_begin(const Dict *mp)
  {
    const_iterator it;
    it.m = mp;
    it.mi = mp ? mp->cbegin() : typename Dict::const_iterator{};
    return it;
  }
  static const_iterator map_end(const Dict *mp)
  {
    const_iterator it;
    it.m = mp;
    it.mi = mp ? mp->cend() : typename Dict::const_iterator{};
    return it;
  }

  reference operator*() const
  {
    if (s)
      return ConstItem{nullptr, (*s)[i]};
    return ConstItem{&mi->first, mi->second};
  }
  const_iterator &operator++()
  {
    if (s)
      ++i;
    else
      ++mi;
    return *this;
  }
  bool operator==(const const_iterator &o) const
  {
    if (s || o.s)
      return s == o.s && i == o.i;
    return m == o.m && mi == o.mi;
  }
  bool operator!=(const const_iterator &o) const
  {
    return !(*this == o);
  }
};

iterator begin()
{
  if (auto sp = std::get_if<Seq>(&data))
    return iterator::seq_begin(sp);
  if (auto mp = std::get_if<Dict>(&data))
    return iterator::map_begin(mp);
  return iterator{};
}
iterator end()
{
  if (auto sp = std::get_if<Seq>(&data))
    return iterator::seq_end(sp);
  if (auto mp = std::get_if<Dict>(&data))
    return iterator::map_end(mp);
  return iterator{};
}
const_iterator begin() const
{
  if (auto sp = std::get_if<Seq>(&data))
    return const_iterator::seq_begin(sp);
  if (auto mp = std::get_if<Dict>(&data))
    return const_iterator::map_begin(mp);
  return const_iterator{};
}
const_iterator end() const
{
  if (auto sp = std::get_if<Seq>(&data))
    return const_iterator::seq_end(sp);
  if (auto mp = std::get_if<Dict>(&data))
    return const_iterator::map_end(mp);
  return const_iterator{};
}
const_iterator cbegin() const
{
  return begin();
}
const_iterator cend() const
{
  return end();
}
