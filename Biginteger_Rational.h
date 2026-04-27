#include <limits.h>

#include <iostream>
#include <string>
#include <vector>

class BigInteger;

bool operator<(const BigInteger& first, const BigInteger& second);
BigInteger operator*(const BigInteger& first, const BigInteger& second);
bool operator<=(const BigInteger& first, const BigInteger& second);
bool operator==(const BigInteger& first, const BigInteger& second);
BigInteger operator/(const BigInteger& first, const BigInteger& second);
BigInteger operator-(const BigInteger& first, const BigInteger& second);
std::ostream& operator<<(std::ostream& output, const BigInteger& other);
bool operator>=(const BigInteger& first, const BigInteger& second);

class BigInteger {
 private:
  int sign = 1;
  int base = 1e9;
  std::vector<long long> digits;

  void check_null() {
    if ((*this) == 0 || -(*this) == 0) {
      sign = 1;
    }
  }

  void get_balance(std::vector<long long>& digits) {
    for (size_t i = 0; i < digits.size() - 1; ++i) {
      digits[i + 1] = digits[i + 1] + digits[i] / base;
      digits[i] = digits[i] % base;
    }
  }

  void increase() {
    size_t i = digits.size() - 1;
    digits.push_back(digits[i] / base);
    digits[i] = digits[i] % base;
  }

  void del_zeros() {
    while (!digits.empty() && digits.back() == 0) {
      digits.pop_back();
    }
    if (digits.empty()) {
      digits.push_back(0);
      sign = 1;
    }
  }

 public:
  BigInteger() = default;

  BigInteger(long long count) {
    if (count < 0) {
      sign = 0;
      count = -count;
    }
    if (count < base) {
      digits.push_back(count);
    } else {
      if (count == 0) {
        digits.push_back(0);
      } else {
        while (count > 0) {
          if (count < base) {
            digits.push_back(count);
            break;
          }
          digits.push_back(count % base);
          count /= base;
        }
      }
    }
  }

  explicit BigInteger(int count) : BigInteger(static_cast<long long>(count)) {}

  explicit BigInteger(unsigned long count)
      : BigInteger(static_cast<long long>(count)) {}

  explicit BigInteger(unsigned long long count)
      : BigInteger(static_cast<long long>(count)) {}

  BigInteger(std::string str) {
    if (str.empty()) {
      sign = 1;
    } else {
      if (str[0] == '-') {
        sign = 0;
        str = str.substr(1);
      } else {
        sign = 1;
      }

      long long cnt = 0;
      for (size_t i = base; i >= 10; i /= 10) {
        ++cnt;
      }

      for (long long i = str.length(); i > 0; i -= cnt) {
        if (i >= cnt) {
          digits.push_back(std::stoi(str.substr(i - cnt, cnt)));
        } else {
          digits.push_back(std::stoi(str.substr(0, i)));
        }
      }

      del_zeros();
    }
  }

  explicit operator bool() {
    if (digits.size() == 1 && digits[0] == 0) {
      return false;
    }
    return true;
  }

  BigInteger operator-() const {
    BigInteger result(*this);
    if (result == 0) {
      result.sign = 0;
    }
    result.sign = 1 - result.sign;
    return result;
  }

  BigInteger operator--(int) {
    BigInteger result(*this);
    *this -= 1;
    return result;
  }

  BigInteger& operator--() { return *this -= 1; }

  BigInteger operator++(int) {
    BigInteger result(*this);
    *this += 1;
    return result;
  }

  BigInteger& operator++() { return *this += 1; }

  BigInteger& operator+=(const BigInteger& other) {
    if (sign == other.sign) {
      if (digits.size() < other.digits.size()) {
        for (size_t i = 0; i < digits.size(); ++i) {
          digits[i] += other.digits[i];
        }
        for (size_t i = digits.size(); i < other.digits.size(); ++i) {
          digits.push_back(other.digits[i]);
        }
        get_balance(digits);

        if (digits[digits.size() - 1] >= base) {
          increase();
        }
      } else {
        for (size_t i = 0; i < other.digits.size(); ++i) {
          digits[i] += other.digits[i];
        }
        get_balance(digits);

        if (digits[digits.size() - 1] >= base) {
          increase();
        }
      }
    } else {
      if ((Abs(*this) < other && sign == 0) ||
          ((*this) < Abs(other) && sign == 1)) {
        BigInteger res(other);
        sign = sign == 0 ? 1 : 0;
        for (size_t i = 0; i < digits.size(); ++i) {
          if (res.digits[i] < digits[i]) {
            if (res.digits[i + 1] != 0) {
              digits[i] = res.digits[i] + base - digits[i];
              res.digits[i + 1] -= 1;
            } else {
              int idx = i + 1;
              while (res.digits[idx] == 0) {
                res.digits[idx] = base - 1;
                ++idx;
              }
              res.digits[idx] -= 1;
              digits[i] = res.digits[i] + base - digits[i];
            }
          } else {
            digits[i] = res.digits[i] - digits[i];
          }
        }
        if (digits.size() < other.digits.size()) {
          for (size_t i = digits.size(); i < res.digits.size(); ++i) {
            digits.push_back(res.digits[i]);
          }
          get_balance(digits);
        }
        del_zeros();
      } else if ((other < Abs(*this) && sign == 0) ||
                 (Abs(other) < (*this) && sign == 1)) {
        for (size_t i = 0; i < other.digits.size(); ++i) {
          if (other.digits[i] > digits[i]) {
            if (digits[i + 1] != 0) {
              digits[i] = digits[i] + base - other.digits[i];
              digits[i + 1] -= 1;
            } else {
              int idx = i + 1;
              while (digits[idx] == 0) {
                digits[idx] = base - 1;
                ++idx;
              }
              digits[idx] -= 1;
              digits[i] = digits[i] + base - other.digits[i];
            }
          } else {
            digits[i] -= other.digits[i];
          }
        }
        del_zeros();
      } else {
        sign = 1;
        digits = {0};
      }
    }
    del_zeros();
    check_null();
    return *this;
  }

  BigInteger& operator-=(const BigInteger& other) {
    (*this) += (-other);
    return *this;
  }

  BigInteger& operator*=(const BigInteger& other) {
    sign = sign == other.sign ? 1 : 0;
    std::vector<long long> mult(digits.size() + other.digits.size() + 1);
    size_t add = 0;
    for (size_t i = 0; i < digits.size(); ++i) {
      for (size_t j = 0; j < other.digits.size(); ++j) {
        mult[i + j] += (digits[i] * other.digits[j] + add);
        add = mult[i + j] / base;
        mult[i + j] %= base;
        if (j == other.digits.size() - 1) {
          mult[i + j + 1] += add;
          if (mult[i + j + 1] >= base) {
            get_balance(mult);
          }
          add = 0;
        }
      }
    }
    digits = mult;
    del_zeros();
    check_null();
    return *this;
  }

  BigInteger& operator/=(const BigInteger& other) {
    del_zeros();
    int flag = (sign == other.sign);
    sign = 1;
    BigInteger remainder = 0;
    BigInteger res = 0;
    for (size_t i = digits.size(); i > 0; --i) {
      remainder *= base;
      remainder.digits[0] += digits[i - 1];
      if (remainder >= other) {
        int l = 0, r = base;
        while (r - l > 1) {
          int mid = (r + l) >> 1;
          if (mid * Abs(other) <= remainder) {
            l = mid;
          } else {
            r = mid;
          }
        }
        remainder -= l * Abs(other);
        res.digits.emplace_back(l);
      } else {
        res.digits.emplace_back(0);
      }
    }
    size_t sz = res.digits.size();
    for (size_t i = 0; i < sz / 2; ++i) {
      std::swap(res.digits[i], res.digits[sz - i - 1]);
    }
    *this = res;
    del_zeros();
    if (digits.size() == 1 && digits[0] == 0)
      sign = 1;
    else
      sign = flag;
    return *this;
  }

  BigInteger& operator%=(const BigInteger& other) {
    *this = (*this - (*this / other) * other);
    del_zeros();
    return *this;
  }

  size_t get_sign() const { return sign; }

  const std::vector<long long> get_digits() const { return digits; }

  size_t get_base() const { return base; }

  std::string toString() const {
    std::string result;
    std::string based = std::to_string(base);
    std::string s1;
    for (size_t i = 0; i < digits.size(); ++i) {
      s1 = std::to_string(digits[i]);
      if (s1.size() < based.size() - 1 && i != digits.size() - 1) {
        size_t n = based.size() - s1.size() - 1;
        for (size_t j = 0; j < n; ++j) {
          s1 = "0" + s1;
        }
      }
      result = s1 + result;
    }
    if (sign == 0) {
      result = "-" + result;
    }
    return result;
  }

  friend BigInteger Abs(const BigInteger& number);
  friend bool operator<(const BigInteger& first, const BigInteger& second);
};

BigInteger operator""_bi(unsigned long long n) { return BigInteger(n); }

BigInteger operator""_bi(const char* str, size_t len) {
  return BigInteger(std::string(str, len));
}

bool operator==(const BigInteger& first, const BigInteger& second) {
  if (first.get_sign() != second.get_sign() ||
      first.get_digits() != second.get_digits()) {
    return false;
  } else {
    return true;
  }
}

bool operator!=(const BigInteger& first, const BigInteger& second) {
  return !(first == second);
}

bool operator<(const BigInteger& first, const BigInteger& second) {
  if (first.get_sign() < second.get_sign()) {
    return true;
  }
  if (first.get_sign() > second.get_sign()) {
    return false;
  }
  if (first.get_sign() == 0) {
    if (first.get_digits().size() < second.get_digits().size()) {
      return false;
    }
    if (first.get_digits().size() > second.get_digits().size()) {
      return true;
    }
    for (long long i = first.get_digits().size() - 1; i >= 0; --i) {
      if (first.get_digits()[i] < second.get_digits()[i]) {
        return false;
      }
      if (first.get_digits()[i] > second.get_digits()[i]) {
        return true;
      }
    }
    return false;
  }
  if (first.get_digits().size() < second.get_digits().size()) {
    return true;
  }
  if (first.get_digits().size() > second.get_digits().size()) {
    return false;
  }
  for (long long i = first.get_digits().size() - 1; i >= 0; --i) {
    if (first.get_digits()[i] < second.get_digits()[i]) {
      return true;
    }
    if (first.get_digits()[i] > second.get_digits()[i]) {
      return false;
    }
  }
  return false;
}

bool operator>(const BigInteger& first, const BigInteger& second) {
  return second < first;
}

bool operator<=(const BigInteger& first, const BigInteger& second) {
  return !(first > second);
}

bool operator>=(const BigInteger& first, const BigInteger& second) {
  return !(first < second);
}

BigInteger operator-(const BigInteger& first, const BigInteger& second) {
  BigInteger result(first);
  result -= second;
  return result;
}

BigInteger Abs(const BigInteger& number) {
  BigInteger res(number);
  res.sign = 1;
  return res;
}

BigInteger operator+(const BigInteger& first, const BigInteger& second) {
  BigInteger res(first);
  res += second;
  return res;
}

BigInteger operator*(const BigInteger& first, const BigInteger& second) {
  BigInteger res(first);
  res *= second;
  return res;
}

BigInteger operator/(const BigInteger& first, const BigInteger& second) {
  BigInteger res(first);
  res /= second;
  return res;
}

BigInteger operator%(const BigInteger& first, const BigInteger& second) {
  BigInteger res(first);
  res %= second;
  return res;
}

std::istream& operator>>(std::istream& input, BigInteger& other) {
  std::string result;
  input >> result;
  other = BigInteger(result);
  return input;
}

std::ostream& operator<<(std::ostream& output, const BigInteger& other) {
  output << other.toString();
  return output;
}

class Rational {
 private:
  BigInteger up = 0_bi;
  BigInteger down = 1_bi;

  BigInteger gcd(BigInteger up, BigInteger down) {
    if (up % down == 0) return down;
    if (down % up == 0) return up;

    if (up > down) {
      up %= down;
      return gcd(up, down);
    }
    down %= up;
    return gcd(up, down);
  }

  void decrease() {
    BigInteger nod = gcd(Abs(up), Abs(down));
    up /= nod;
    down /= nod;
  }

  void change_sign() {
    if (up.get_sign() == 0 && up.get_sign() == down.get_sign()) {
      up = -up;
      down = -down;
    }
    if (up.get_sign() == 1 && up.get_sign() != down.get_sign()) {
      down = -down;
      up = -up;
    }
  }

 public:
  Rational() {};

  Rational(BigInteger big) {
    up = big;
    down = 1_bi;
  }

  Rational(int big) {
    up = BigInteger(big);
    down = 1_bi;
  }

  Rational operator-() const {
    Rational other(*this);
    other.up = -other.up;
    return other;
  }

  Rational& operator+=(const Rational& other) {
    up = up * other.down + down * other.up;
    down = down * other.down;
    decrease();
    change_sign();
    return *this;
  }

  Rational& operator-=(const Rational& other) {
    *this += (-other);
    return *this;
  }

  Rational& operator*=(const Rational& other) {
    up = up * other.up;
    down = down * other.down;
    decrease();
    change_sign();
    return *this;
  }

  Rational& operator/=(const Rational& other) {
    up = up * other.down;
    down = down * other.up;
    decrease();
    change_sign();
    return *this;
  }

  friend bool operator==(const Rational& first, const Rational& second);

  friend bool operator<(const Rational& first, const Rational& second);

  std::string toString() const {
    if (down == 1) {
      return up.toString();
    }
    return up.toString() + "/" + down.toString();
  }

  std::string asDecimal(size_t pres = 0) const {
    std::string result;
    if (up < BigInteger(0)) {
      result.push_back('-');
    }
    result.append((Abs(up) / down).toString());
    BigInteger ost = Abs(up) % down;
    if (pres == 0) {
      return result;
    } else {
      result.push_back('.');
    }
    while (pres != 0) {
      ost *= 10_bi;
      result.append((ost / down).toString());
      ost %= down;
      pres--;
    }
    return result;
  }

  explicit operator double() const { return stod(asDecimal(23)); }
};

Rational operator+(const Rational& first, const Rational& second) {
  Rational res(first);
  res += second;
  return res;
}

Rational operator*(const Rational& first, const Rational& second) {
  Rational res(first);
  res *= second;
  return res;
}

Rational operator/(const Rational& first, const Rational& second) {
  Rational res(first);
  res /= second;
  return res;
}

Rational operator-(const Rational& first, const Rational& second) {
  Rational res(first);
  res -= second;
  return res;
}

bool operator==(const Rational& first, const Rational& second) {
  return first.up == second.up && first.down == second.down;
}

bool operator!=(const Rational& first, const Rational& second) {
  return !(first == second);
}

bool operator<(const Rational& first, const Rational& second) {
  return first.up < second.up;
}

bool operator>(const Rational& first, const Rational& second) {
  return second < first;
}

bool operator<=(const Rational& first, const Rational& second) {
  return !(first > second);
}

bool operator>=(const Rational& first, const Rational& second) {
  return !(first < second);
}

std::ostream& operator<<(std::ostream& output, const Rational& other) {
  output << other.toString();
  return output;
}
