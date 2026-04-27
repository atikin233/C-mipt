#include <algorithm>
#include <cstring>
#include <iostream>

class String {
 private:
  char* str;
  size_t sz;
  size_t cap;

  size_t LeftOrRight(bool flag, const String& c) const {
    if (c.sz > sz) return sz;
    long long i;
    int n;
    if (flag == false) {
      i = sz - c.sz;
      n = -1;
    } else {
      i = 0;
      n = 1;
    }
    while (i <= (long long)(sz - c.sz) && i >= 0) {
      bool found = true;
      for (size_t j = 0; j < c.sz; ++j) {
        if (str[i + j] != c[j]) {
          found = false;
          break;
        }
      }
      if (found) return i;
      i += n;
    }
    return sz;
  }

 public:
  String() : str(new char[1]), sz(0), cap(0) { str[0] = '\0'; }

  String(size_t count) : str(new char[count + 1]), sz(0), cap(count) {
    str[count] = '\0';
  }

  String(size_t count, char c)
      : str(new char[count + 1]), sz(count), cap(count) {
    std::fill(str, str + sz, c);
    str[sz] = '\0';
  }

  String(const String& other)
      : str(new char[other.cap + 1]), sz(other.sz), cap(other.cap) {
    std::copy(other.str, other.str + sz, str);
    str[sz] = '\0';
  }

  String(const char* other)
      : str(new char[std::strlen(other) + 1]),
        sz(std::strlen(other)),
        cap(std::strlen(other)) {
    std::copy(other, other + sz, str);
    str[sz] = '\0';
  }

  ~String() { delete[] str; }

  String& operator=(const String& other) {
    if (this == &other) {
      return *this;
    }
    if (cap < other.cap) {
      delete[] str;
      str = new char[other.cap + 1];
      sz = other.sz;
      cap = other.cap;
      std::copy(other.str, other.str + sz, str);
      str[sz] = '\0';
      return *this;
    } else {
      sz = other.sz;
      std::copy(other.str, other.str + sz, str);
      str[sz] = '\0';
      return *this;
    }
  }

  String& operator+=(const String& other) {
    if (cap < sz + other.sz) {
      char* new_str = new char[sz + other.sz + 1];
      std::copy(str, str + sz, new_str);
      cap = sz + other.sz;
      delete[] str;
      str = new_str;
    }
    std::copy(other.str, other.str + other.sz, str + sz);
    sz += other.sz;
    str[sz] = '\0';
    return *this;
  }

  String& operator+=(char c) {
    push_back(c);
    return *this;
  }

  char& operator[](size_t index) { return str[index]; }

  const char& operator[](size_t index) const { return str[index]; }

  size_t size() const { return sz; }

  size_t length() const { return sz; }

  size_t capacity() const { return cap; }

  bool empty() const {
    if (sz == 0) {
      return true;
    }
    return false;
  }

  void shrink_to_fit() {
    if (cap > sz) {
      char* new_str = new char[sz + 1];
      std::copy(str, str + sz, new_str);
      new_str[sz] = '\0';
      delete[] str;
      str = new_str;
      cap = sz;
    }
  }

  void clear() {
    str[0] = '\0';
    sz = 0;
  }

  void push_back(char c) {
    if (sz < cap) {
      str[sz] = c;
      ++sz;
      str[sz] = '\0';
    } else {
      char* new_str;
      if (cap == 0) {
        new_str = new char[2];
        cap = 1;
      } else {
        new_str = new char[cap * 2 + 1];
        std::copy(str, str + sz, new_str);
        cap = cap * 2;
      }
      new_str[sz] = c;
      new_str[sz + 1] = '\0';
      delete[] str;
      str = new_str;
      ++sz;
    }
  }

  void pop_back() {
    str[sz - 1] = '\0';
    --sz;
  }

  char& front() { return str[0]; }

  const char& front() const { return str[0]; }

  char& back() { return str[sz - 1]; }

  const char& back() const { return str[sz - 1]; }

  char* data() const { return str; }

  String substr(size_t start, size_t count) const {
    if (start + count > sz) count = sz - start;
    if (start >= sz) count = 0;
    String result(count, '\0');
    std::copy(str + start, str + start + count, result.str);
    return result;
  }

  size_t find(const String& c) const { return LeftOrRight(true, c); }

  size_t rfind(const String& c) const { return LeftOrRight(false, c); }

  friend String operator+(char first, const String& second);
};

bool operator==(const String& first, const String& second) {
  if (first.size() != second.size()) {
    return false;
  } else {
    for (size_t i = 0; i < first.size(); ++i) {
      if (first[i] != second[i]) {
        return false;
      }
    }
    return true;
  }
}

bool operator!=(const String& first, const String& second) {
  return !(first == second);
}

bool operator<(const String& first, const String& second) {
  size_t length = std::min(first.size(), second.size());
  for (size_t i = 0; i < length; ++i) {
    if (first[i] > second[i]) {
      return false;
    }
    if (first[i] < second[i]) {
      return true;
    }
  }
  if (first.size() >= second.size()) {
    return false;
  }
  return true;
}

bool operator>(const String& first, const String& second) {
  return second < first;
}

bool operator<=(const String& first, const String& second) {
  return !(first > second);
}

bool operator>=(const String& first, const String& second) {
  return !(first < second);
}

String operator+(const String& first, const String& second) {
  String result = first;
  return result += second;
}

String operator+(const String& first, char second) {
  String result = first;
  return result += second;
}

String operator+(char first, const String& second) {
  String result(second.sz + 1);
  result[0] = first;
  result.sz += 1;
  return result += second;
}

std::istream& operator>>(std::istream& is, String& s) {
  s.clear();
  char c;
  while (is.get(c) && (!std::isspace(c))) {
    s += c;
  }
  return is;
}
std::ostream& operator<<(std::ostream& os, const String& s) {
  for (size_t i = 0; i < s.size(); ++i) {
    os << s[i];
  }
  return os;
}
