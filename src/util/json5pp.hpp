//
//  json5pp.hpp
//  Player
//
//  Created by ゾロアーク on 11/21/20.
//

// https://github.com/kimushu/json5pp

#ifndef _JSON5PP_HPP_
#define _JSON5PP_HPP_

#include <stdexcept>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <initializer_list>
#include <cmath>
#include <limits>
#include <streambuf>

namespace json5pp {

/*
 * json5pp uses semantic versioning
 * See: https://semver.org/
 */
namespace version {
  static constexpr auto major = 2;
  static constexpr auto minor = 2;
  static constexpr auto patch = 0;
}

/**
 * @class syntax_error
 * A class of objects thrown as exceptions to report a JSON syntax error.
 */
class syntax_error : public std::invalid_argument
{
public:
  /**
   * Construct syntax_error object with failed character.
   * @param ch Character which raised error or Traits::eof()
   * @param context_name Context description in human-readable string
   */
  syntax_error(int ch, const char *context_name)
  : std::invalid_argument(
      std::string("JSON syntax error: ") +
      (ch != std::char_traits<char>::eof() ?
        "illegal character `" + std::string(1, static_cast<char>(ch)) + "'" :
        "unexpected EOS") +
      " in " + context_name) {}
};

namespace impl {

/**
 * @brief Parser/stringifier flags
 */
enum flags : std::uint32_t
{
  // Syntax flags
  single_line_comment     = (1u<<0),
  multi_line_comment      = (1u<<1),
  comments                = single_line_comment|multi_line_comment,
  explicit_plus_sign      = (1u<<2),
  leading_decimal_point   = (1u<<3),
  trailing_decimal_point  = (1u<<4),
  decimal_points          = leading_decimal_point|trailing_decimal_point,
  infinity_number         = (1u<<5),
  not_a_number            = (1u<<6),
  hexadecimal             = (1u<<7),
  single_quote            = (1u<<8),
  multi_line_string       = (1u<<9),
  trailing_comma          = (1u<<10),
  unquoted_key            = (1u<<11),

  // Syntax flag sets
  json5_rules             = ((unquoted_key<<1)-1),
  all_rules               = json5_rules,

  // Parse options
  finished                = (1u<<29),
  parse_mask              = all_rules|finished,

  // Stringify options
  crlf_newline            = (1u<<31),
  stringify_mask          = infinity_number|not_a_number|crlf_newline,
};

using flags_type = std::underlying_type<flags>::type;
using indent_type = std::int8_t;

template <flags_type F> class parser;
template <flags_type F, indent_type I> class stringifier;

template <flags_type S, flags_type C>
class manipulator_flags
{
public:
  /**
   * @brief Invert set/clear of flags
   *
   * @return A new flag manipulator
   */
  manipulator_flags<C,S> operator-() const { return manipulator_flags<C,S>(); }

  template <flags_type S_, flags_type C_>
  friend parser<S_&flags::parse_mask> operator>>(std::istream& istream, const manipulator_flags<S_,C_>& manip);

  template <flags_type S_, flags_type C_>
  friend stringifier<S_&flags::stringify_mask,0> operator<<(std::ostream& ostream, const manipulator_flags<S_,C_>& manip);
};

template <indent_type I>
class manipulator_indent
{
public:
  template <indent_type I_>
  friend parser<0> operator>>(std::istream& istream, const manipulator_indent<I_>& manip);

  template <indent_type I_>
  friend stringifier<0,I_> operator<<(std::ostream& ostream, const manipulator_indent<I_>& manip);
};

} /* namespace impl */

/**
 * @brief A class to hold JSON value
 */
class value
{
private:
  enum type_enum {
    TYPE_NULL,
    TYPE_BOOLEAN,
    TYPE_NUMBER,
    TYPE_STRING,
    TYPE_ARRAY,
    TYPE_OBJECT,
    TYPE_INTEGER,
  } type;

public:
  using null_type = std::nullptr_t;
  using boolean_type = bool;
  using number_type = double;
  using integer_type = int;
  using number_i_type = integer_type;
  using string_type = std::string;
  using string_p_type = const char*;
  using array_type = std::vector<value>;
  using object_type = std::map<string_type, value>;
  using pair_type = object_type::value_type;
  using json_type = std::string;

  /*================================================================================
   * Construction
   */
public:
  /**
   * @brief JSON value default constructor for "null" type.
   */
  value() noexcept : value(nullptr) {}

  /**
   * @brief JSON value constructor for "null" type.
   * @param null A dummy argument for nullptr
   */
  value(null_type null) noexcept : type(TYPE_NULL) {}

  /**
   * @brief JSON value constructor for "boolean" type.
   * @param boolean A boolean value to be set.
   */
  value(boolean_type boolean) noexcept : type(TYPE_BOOLEAN), content(boolean) {}

  /**
   * @brief JSON value constructor for "number" type.
   * @param number A number to be set.
   */
  value(number_type number) noexcept : type(TYPE_NUMBER), content(number) {}

  /**
   * @brief JSON value constructor with integer for "number" type.
   * @param number An integer value to be set.
   */
  value(integer_type integer) noexcept : type(TYPE_INTEGER), content(integer) {}

  /**
   * @brief JSON value constructor for "string" type.
   * @param string A string value to be set.
   */
  value(const string_type& string) : type(TYPE_STRING)
  {
    new(&content.string) string_type(string);
  }

  /**
   * @brief JSON value constructor for "string" type. (const char* version)
   * @param string A string value to be set.
   */
  value(string_p_type string) : type(TYPE_STRING)
  {
    new(&content.string) string_type(string);
  }

  /**
   * @brief JSON value constructor for "array" type.
   * @param array An initializer list of elements.
   */
  explicit value(std::initializer_list<value> array) : type(TYPE_ARRAY)
  {
    new(&content.array) array_type(array);
  }

  /**
   * @brief JSON value constructor with key,value pair for "object" type.
   * @param elements An initializer list of key,value pair.
   */
  explicit value(std::initializer_list<pair_type> elements) : type(TYPE_OBJECT)
  {
    new(&content.object) object_type(elements);
  }

  /**
   * @brief JSON value copy constructor.
   * @param src A value to be copied from.
   */
  value(const value& src) : type(TYPE_NULL)
  {
    *this = src;
  }

  /**
   * @brief JSON value move constructor.
   * @param src A value to be moved from.
   */
  value(value&& src) : type(src.type)
  {
    switch (type) {
    case TYPE_BOOLEAN:
      new (&content.boolean) boolean_type(std::move(src.content.boolean));
      break;
    case TYPE_NUMBER:
      new (&content.number) number_type(std::move(src.content.number));
      break;
    case TYPE_INTEGER:
      new (&content.integer) integer_type(std::move(src.content.integer));
      break;
    case TYPE_STRING:
      new (&content.string) string_type(std::move(src.content.string));
      break;
    case TYPE_ARRAY:
      new (&content.array) array_type(std::move(src.content.array));
      break;
    case TYPE_OBJECT:
      new (&content.object) object_type(std::move(src.content.object));
      break;
    default:
      break;
    }
    src.type = TYPE_NULL;
  }

  friend value array(std::initializer_list<value> elements);
  friend value object(std::initializer_list<pair_type> elements);

  /*================================================================================
   * Destruction
   */
public:
  /**
   * @brief JSON value destructor.
   */
  ~value()
  {
    release();
  }

private:
  /**
   * @brief Release content
   *
   * @param new_type A new type id
   */
  void release(type_enum new_type = TYPE_NULL)
  {
    switch (type) {
    case TYPE_BOOLEAN:
      content.boolean.~boolean_type();
      break;
    case TYPE_NUMBER:
      content.number.~number_type();
      break;
    case TYPE_INTEGER:
      content.integer.~integer_type();
      break;
    case TYPE_STRING:
      content.string.~string_type();
      break;
    case TYPE_ARRAY:
      content.array.~array_type();
      break;
    case TYPE_OBJECT:
      content.object.~object_type();
      break;
    default:
      break;
    }
    type = new_type;
  }

  /*================================================================================
   * Type checks
   */
public:
  /**
   * @brief Check if stored value is null.
   */
  bool is_null() const noexcept { return type == TYPE_NULL; }

  /**
   * @brief Check if type of stored value is boolean.
   */
  bool is_boolean() const noexcept { return type == TYPE_BOOLEAN; }

  /**
   * @brief Check if type of stored value is number (includes integer).
   */
  bool is_number() const noexcept { return (type == TYPE_NUMBER) || (type == TYPE_INTEGER); }

  /**
   * @brief Check if type of stored value is integer.
   */
  bool is_integer() const noexcept { return type == TYPE_INTEGER; }

  /**
   * @brief Check if type of stored value is string.
   */
  bool is_string() const noexcept { return type == TYPE_STRING; }

  /**
   * @brief Check if type of stored value is array.
   */
  bool is_array() const noexcept { return type == TYPE_ARRAY; }

  /**
   * @brief Check if type of stored value is object.
   */
  bool is_object() const noexcept { return type == TYPE_OBJECT; }

  /*================================================================================
   * Type casts
   */
public:
  /**
   * @brief Cast to null
   *
   * @throws std::bad_cast if the value is not a null
   */
  null_type as_null() const
  {
    if (type != TYPE_NULL) { throw std::bad_cast(); }
    return nullptr;
  }

  /**
   * @brief Cast to boolean
   *
   * @throws std::bad_cast if the value is not a boolean
   */
  boolean_type as_boolean() const
  {
    if (type != TYPE_BOOLEAN) { throw std::bad_cast(); }
    return content.boolean;
  }

  /**
   * @brief Cast to number
   *
   * @throws std::bad_cast if the value is not a number nor integer
   */
  number_type as_number() const
  {
    if (type == TYPE_INTEGER) {
      return static_cast<number_type>(content.integer);
    } else if (type != TYPE_NUMBER) {
      throw std::bad_cast();
    }
    return content.number;
  }

  /**
   * @brief Cast to integer number
   *
   * @throws std::bad_cast if the value is not a number nor integer
   */
  integer_type as_integer() const
  {
    if (type == TYPE_NUMBER) {
      return static_cast<integer_type>(content.number);
    } else if (type != TYPE_INTEGER) {
      throw std::bad_cast();
    }
    return content.integer;
  }

  /**
   * @brief Cast to string
   *
   * @throws std::bad_cast if the value is not a string
   */
  const string_type& as_string() const
  {
    if (type != TYPE_STRING) { throw std::bad_cast(); }
    return content.string;
  }

  /**
   * @brief Cast to string reference
   *
   * @throws std::bad_cast if the value is not a string
   */
  string_type& as_string()
  {
    if (type != TYPE_STRING) { throw std::bad_cast(); }
    return content.string;
  }

  /**
   * @brief Cast to array
   *
   * @throws std::bad_cast if the value is not a array
   */
  const array_type& as_array() const
  {
    if (type != TYPE_ARRAY) { throw std::bad_cast(); }
    return content.array;
  }

  /**
   * @brief Cast to array reference
   *
   * @throws std::bad_cast if the value is not a array
   */
  array_type& as_array()
  {
    if (type != TYPE_ARRAY) { throw std::bad_cast(); }
    return content.array;
  }

  /**
   * @brief Cast to object
   *
   * @throws std::bad_cast if the value is not a object
   */
  const object_type& as_object() const
  {
    if (type != TYPE_OBJECT) { throw std::bad_cast(); }
    return content.object;
  }

  /**
   * @brief Cast to object reference
   *
   * @throws std::bad_cast if the value is not a object
   */
  object_type& as_object()
  {
    if (type != TYPE_OBJECT) { throw std::bad_cast(); }
    return content.object;
  }

  /*================================================================================
   * Truthy/falsy test
   */
  operator bool() const
  {
    switch (type) {
    case TYPE_BOOLEAN:
      return content.boolean;
    case TYPE_NUMBER:
      return (content.number != 0) && (!std::isnan(content.number));
    case TYPE_INTEGER:
      return (content.integer != 0);
    case TYPE_STRING:
      return !content.string.empty();
    case TYPE_ARRAY:
    case TYPE_OBJECT:
      return true;
    case TYPE_NULL:
    default:
      return false;
    }
  }

  /*================================================================================
   * Array indexer
   */
  const value& at(const int index, const value& default_value) const
  {
    if (type == TYPE_ARRAY) {
      if ((0 <= index) && (index < (int)content.array.size())) {
        return content.array[index];
      }
    }
    return default_value;
  }

  const value& at(const int index) const
  {
    static const value null;
    return at(index, null);
  }

  const value& operator[](const int index) const
  {
    return at(index);
  }

  /*================================================================================
   * Object indexer
   */
  const value& at(const string_type& key, const value& default_value) const
  {
    if (type == TYPE_OBJECT) {
      auto iter = content.object.find(key);
      if (iter != content.object.end()) {
        return iter->second;
      }
    }
    return default_value;
  }

  const value& at(const string_type& key) const
  {
    static const value null;
    return at(key, null);
  }

  const value& operator[](const string_type& key) const
  {
    return at(key);
  }

  const value& at(const string_p_type key, const value& default_value) const
  {
    return at(string_type(key), default_value);
  }

  const value& at(const string_p_type key) const
  {
    return at(string_type(key));
  }

  const value& operator[](const string_p_type key) const
  {
    return at(string_type(key));
  }

  /*================================================================================
   * Assignment (Copying)
   */
public:
  /**
   * @brief Copy from another JSON value object.
   * @param src A value object.
   */
  value& operator=(const value& src)
  {
    release(src.type);
    switch (type) {
    case TYPE_BOOLEAN:
      new (&content.boolean) boolean_type(src.content.boolean);
      break;
    case TYPE_NUMBER:
      new (&content.number) number_type(src.content.number);
      break;
    case TYPE_INTEGER:
      new (&content.integer) integer_type(src.content.integer);
      break;
    case TYPE_STRING:
      new (&content.string) string_type(src.content.string);
      break;
    case TYPE_ARRAY:
      new (&content.array) array_type(src.content.array);
      break;
    case TYPE_OBJECT:
      new (&content.object) object_type(src.content.object);
      break;
    default:
      break;
    }
    return *this;
  }

  /**
   * @brief Assign null value.
   * @param null A dummy value.
   */
  value& operator=(null_type null)
  {
    release();
    return *this;
  }

  /**
   * @brief Assign boolean value.
   * @param boolean A boolean value to be set.
   */
  value& operator=(boolean_type boolean)
  {
    release(TYPE_BOOLEAN);
    new (&content.boolean) boolean_type(boolean);
    return *this;
  }

  /**
   * @brief Assign number value.
   * @param number A number to be set.
   */
  value& operator=(number_type number)
  {
    release(TYPE_NUMBER);
    new (&content.number) number_type(number);
    return *this;
  }

  /**
   * @brief Assign number value by integer type.
   * @param integer A integer number to be set.
   */
  value& operator=(integer_type integer)
  {
    release(TYPE_INTEGER);
    new (&content.integer) integer_type(integer);
    return *this;
  }

  /**
   * @brief Assign string value.
   * @param string A string to be set.
   */
  value& operator=(const string_type& string)
  {
    if (type == TYPE_STRING) {
      content.string = string;
    } else {
      release(TYPE_STRING);
      new (&content.string) string_type(string);
    }
    return *this;
  }

  /**
   * @brief Assign string value from const char*
   * @param string A string to be set.
   */
  value& operator=(string_p_type string)
  {
    return (*this = string_type(string));
  }

  /**
   * @brief Assign array value by deep copy.
   * @param array An array to be set.
   */
  value& operator=(std::initializer_list<value> array)
  {
    if (type == TYPE_ARRAY) {
      content.array = array;
    } else {
      release(TYPE_ARRAY);
      new (&content.array) array_type(array);
    }
    return *this;
  }

  /**
   * @brief Assign object value by deep copy.
   * @param object An object to be set.
   */
  value& operator=(std::initializer_list<value::pair_type> elements)
  {
    if (type == TYPE_OBJECT) {
      content.object = elements;
    } else {
      release(TYPE_OBJECT);
      new (&content.object) object_type(elements);
    }
    return *this;
  }

  /*================================================================================
   * Parse
   */
private:
  template <impl::flags_type F>
  friend class impl::parser;

  friend impl::parser<0> operator>>(std::istream& istream, value& v);

  /*================================================================================
   * Stringify
   */
  template <impl::flags_type F, impl::indent_type I>
  friend class impl::stringifier;

  friend impl::stringifier<0,0> operator<<(std::ostream& ostream, const value& v);

public:
  template <class... T>
  json_type stringify(T... args) const;

  template <class... T>
  json_type stringify5(T... args) const;

  /*================================================================================
   * Internal data structure
   */
private:
  union content {
    boolean_type boolean;
    number_type number;
    integer_type integer;
    string_type string;
    array_type array;
    object_type object;
    content() {}
    content(boolean_type boolean) : boolean(boolean) {}
    content(number_type number) : number(number) {}
    content(integer_type integer) : integer(integer) {}
    ~content() {}
  } content;
};

/**
 * @brief Make JSON array
 *
 * @param elements An initializer list of elements
 * @return JSON value object
 */
inline value array(std::initializer_list<value> elements)
{
  value v;
  v.release(value::TYPE_ARRAY);
  new (&v.content.array) value::array_type(elements);
  return v;
}

/**
 * @brief Make JSON object
 *
 * @param elements An initializer list of key:value pairs
 * @return JSON value object
 */
inline value object(std::initializer_list<value::pair_type> elements)
{
  value v;
  v.release(value::TYPE_OBJECT);
  new (&v.content.object) value::object_type(elements);
  return v;
}

namespace impl {

/**
 * @brief Parser implementation
 *
 * @tparam F A combination of flags
 */
template <flags_type F>
class parser
{
private:
  using self_type = parser<F>;
  static constexpr auto M = flags::parse_mask;

public:
  /**
   * @brief Construct a new parser object
   *
   * @param istream An input stream
   */
  parser(std::istream& istream) : istream(istream) {}

  /**
   * @brief Apply flag manipulator
   *
   * @tparam S Flags to be set
   * @tparam C Flags to be cleared
   * @param manip A manipulator
   * @return An updated parser object
   */
  template <flags_type S, flags_type C>
  parser<((F&~C)|S)&M> operator>>(const manipulator_flags<S,C>& manip)
  {
    return parser<((F&~C)|S)&M>(istream);
  }

  /**
   * @brief Apply indent manipulator (For parser, this is ignored)
   *
   * @tparam NI A new indent specification
   * @param manip A manipulator
   * @return A reference to self
   */
  template <indent_type NI>
  self_type& operator>>(const manipulator_indent<NI>& manip)
  {
    return *this;
  }

  /**
   * @brief Delegate manipulator to std::istream
   *
   * @param manip A stream manipulator
   * @return An input stream
   */
  std::istream& operator>>(std::istream& (*manip)(std::istream&))
  {
    return istream >> manip;
  }

  /**
   * @brief Delegate operator>> to std::istream
   *
   * @tparam T A typename of argument
   * @param v A value
   * @return An input stream
   */
  template <class T>
  std::istream& operator>>(T& v)
  {
    return istream >> v;
  }

  /**
   * @brief Parse JSON
   *
   * @param v A value object to store parsed value
   * @return A reference to self
   */
  self_type& operator>>(value& v)
  {
    do_parse(v);
    return *this;
  }

private:
  /**
   * @brief Check if flag(s) enabled
   *
   * @param flags Combination of flags to be tested
   * @retval true Any flag is enabled
   * @retval False No flag is enabled
   */
  static constexpr bool has_flag(flags_type flags)
  {
    return (F & flags) != 0;
  }

  /**
   * @brief Skip spaces (and comments) from input stream
   *
   * @return the first non-space character
   */
  int skip_spaces()
  {
    for (;;) {
      int ch = istream.get();
    reeval_space:
      switch (ch) {
      case '\t':
      case '\n':
      case '\r':
      case ' ':
        continue;
      case '/':
        if (has_flag(flags::single_line_comment | flags::multi_line_comment)) {
          ch = istream.get();
          if (has_flag(flags::single_line_comment) && (ch == '/')) {
            // [single_line_comment] (JSON5)
            for (;;) {
              ch = istream.get();
              if ((ch == std::char_traits<char>::eof()) || (ch == '\r') || (ch == '\n')) {
                break;
              }
            }
            goto reeval_space;
          } else if (has_flag(flags::multi_line_comment) && (ch == '*')) {
            // [multi_line_comment] (JSON5)
            for (;;) {
              ch = istream.get();
            reeval_asterisk:
              if (ch == std::char_traits<char>::eof()) {
                throw syntax_error(ch, "comment");
              }
              if (ch != '*') {
                continue;
              }
              ch = istream.get();
              if (ch == '*') {
                goto reeval_asterisk;
              }
              if (ch == '/') {
                break;
              }
            }
            continue;
          }
          // no valid comments
        }
        /* no-break */
      default:
        return ch;
      } /* switch(ch) */
    } /* for(;;) */
  }

  /**
   * @brief Check if a character is a digit [0-9]
   *
   * @param ch Character code to test
   * @retval true The character is a digit
   * @retval false The character is not a digit
   */
  static bool is_digit(int ch)
  {
    return (('0' <= ch) && (ch <= '9'));
  }

  /**
   * @brief Convert a digit character [0-9] to number (0-9)
   *
   * @param ch Character code to convert
   * @return A converted number (0-9)
   */
  static int to_number(int ch)
  {
    return ch - '0';
  }

  /**
   * @brief Convert a hexadecimal digit character [0-9A-Fa-f] to number (0-15)
   *
   * @param ch Character code to convert
   * @return int A converted number (0-15)
   */
  static int to_number_hex(int ch)
  {
    if (is_digit(ch)) {
      return to_number(ch);
    } else if (('A' <= ch) && (ch <= 'F')) {
      return ch - 'A' + 10;
    } else if (('a' <= ch) && (ch <= 'f')) {
      return ch - 'a' + 10;
    }
    return -1;
  }

  /**
   * @brief Check if a character is an alphabet
   *
   * @param ch
   * @return true
   * @return false
   */
  static bool is_alpha(int ch)
  {
    return (('A' <= ch) && (ch <= 'Z')) || (('a' <= ch) && (ch <= 'z'));
  }

  /**
   * @brief Check character sequence
   *
   * @tparam C A list of typenames of character
   * @param ch A buffer to store latest character code
   * @param expected The first expected character code
   * @param sequence Successive expected character codes
   * @retval true All characters match
   * @return false Mismatch (ch holds a mismatched character)
   */
  template <class... C>
  bool equals(int& ch, char expected, C... sequence)
  {
    return equals(ch, expected) && equals(ch, sequence...);
  }
  bool equals(int& ch, char expected)
  {
    return ((ch = istream.get()) == expected);
  }

  /**
   * @brief Parser entry
   *
   * @param v A value object to store parsed value
   */
  void do_parse(value& v)
  {
    static const char context[] = "value";
    parse_value(v, context);
    if (F & flags::finished) {
      int ch = skip_spaces();
      if (ch != std::char_traits<char>::eof()) {
        throw syntax_error(ch, context);
      }
    }
  }

  /**
   * @brief Parse value
   *
   * @param v A value object to store parsed value
   * @param context A description of context
   */
  void parse_value(value& v, const char *context)
  {
    int ch = skip_spaces();

    // [value]
    switch (ch) {
    case '{':
      // [object]
      return parse_object(v);
    case '[':
      // [array]
      return parse_array(v);
    case '"':
    case '\'':
      // [string]
      return parse_string(v, ch);
    case 'n':
      // ["null"]?
      return parse_null(v);
    case 't':
    case 'f':
      // ["true"] or ["false"]?
      return parse_boolean(v, ch);
    default:
      if (is_digit(ch) || (ch == '-') || (ch == '+') ||
          (ch == '.') || (ch == 'i') || (ch == 'N')) {
        // [number]?
        return parse_number(v, ch);
      }
      throw syntax_error(ch, context);
    }
  }

  /**
   * @brief Parse null value
   *
   * @param v A value object to store parsed value
   */
  void parse_null(value& v)
  {
    static const char context[] = "null";
    int ch;
    if (equals(ch, 'u', 'l', 'l')) {
      v = nullptr;
      return;
    }
    throw syntax_error(ch, context);
  }

  /**
   * @brief Parse boolean value
   *
   * @param v A value object to store parsed value
   * @param ch The first character
   */
  void parse_boolean(value& v, int ch)
  {
    static const char context[] = "boolean";
    if (ch == 't') {
      if (equals(ch, 'r', 'u', 'e')) {
        v = true;
        return;
      }
    } else if (ch == 'f') {
      if (equals(ch, 'a', 'l', 's', 'e')) {
        v = false;
        return;
      }
    }
    throw syntax_error(ch, context);
  }

  /**
   * @brief Parse number value
   *
   * @param v A value object to store parsed value
   * @param ch The first character
   */
  void parse_number(value& v, int ch)
  {
    static const char context[] = "number";
    unsigned long long int_part = 0;
    unsigned long long frac_part = 0;
    int frac_divs = 0;
    int exp_part = 0;
    bool exp_negative = false;
    bool negative = false;

    // [int]
    if (ch == '-') {
      negative = true;
      ch = istream.get();
    } else if (has_flag(flags::explicit_plus_sign) && (ch == '+')) {
      ch = istream.get();
    }
    // [digit|digits]
    for (;;) {
      if (ch == '0') {
        // ["0"]
        ch = istream.get();
        if (has_flag(flags::hexadecimal) && ((ch == 'x') || (ch == 'X'))) {
          // [hexdigit]+
          bool no_digit = true;
          for (;;) {
            ch = istream.get();
            int digit = to_number_hex(ch);
            if (digit < 0) {
              istream.unget();
              break;
            }
            int_part = (int_part << 4) | digit;
            no_digit = false;
          }
          if (no_digit) {
            throw syntax_error(ch, context);
          }
          v = negative ? (double)(-(long long)int_part) : (double)int_part;
          return;
        }
        break;
      } else if (is_digit(ch)) {
        // [onenine]
        int_part = to_number(ch);
        for (; ch = istream.get(), is_digit(ch);) {
          int_part *= 10;
          int_part += to_number(ch);
        }
        break;
      } else if (has_flag(flags::leading_decimal_point) && (ch == '.')) {
        // ['.'] (JSON5)
        break;
      } else if (has_flag(flags::infinity_number) && (ch == 'i')) {
        // ["infinity"] (JSON5)
        if (equals(ch, 'n', 'f', 'i', 'n', 'i', 't', 'y')) {
          v = negative ?
            -std::numeric_limits<value::number_type>::infinity() :
            +std::numeric_limits<value::number_type>::infinity();
          return;
        }
      } else if (has_flag(flags::not_a_number) && (ch == 'N')) {
        // ["NaN"] (JSON5)
        if (equals(ch, 'a', 'N')) {
          v = std::numeric_limits<value::number_type>::quiet_NaN();
          return;
        }
      }
      throw syntax_error(ch, context);
    }
    if (ch == '.') {
      // [frac]
      for (; ch = istream.get(), is_digit(ch); ++frac_divs) {
        frac_part *= 10;
        frac_part += to_number(ch);
      }
      if ((!has_flag(flags::trailing_decimal_point)) && (frac_divs == 0)) {
        throw syntax_error(ch, context);
      }
    }
    if ((ch == 'e') || (ch == 'E')) {
      // [exp]
      ch = istream.get();
      switch (ch) {
      case '-':
        exp_negative = true;
        /* no-break */
      case '+':
        ch = istream.get();
        break;
      }
      bool no_digit = true;
      for (; is_digit(ch); no_digit = false, ch = istream.get()) {
        exp_part *= 10;
        exp_part += to_number(ch);
      }
      if (no_digit) {
        throw syntax_error(ch, context);
      }
    }
    istream.unget();
    if ((frac_part == 0) && (exp_part == 0)) {
      if (negative) {
        const auto integer_value = static_cast<value::integer_type>(-int_part);
        if (static_cast<decltype(int_part)>(integer_value) == -int_part) {
          v = integer_value;
          return;
        }
      } else {
        const auto integer_value = static_cast<value::integer_type>(int_part);
        if (static_cast<decltype(int_part)>(integer_value) == int_part) {
          v = integer_value;
          return;
        }
      }
    }
    double number_value = (double)int_part;
    if (frac_part > 0) {
      number_value += (static_cast<double>(frac_part) * std::pow(10, -frac_divs));
    }
    if (exp_part > 0) {
      number_value *= std::pow(10, exp_negative ? -exp_part : +exp_part);
    }
    v = negative ? -number_value : +number_value;
  }

  /**
   * @brief Parse string
   *
   * @param buffer A buffer to store string
   * @param quote The first quote character
   * @param context A description of context
   */
  void parse_string(std::string& buffer, int quote, const char *context)
  {
    if (!((quote == '"') || (has_flag(flags::single_quote) && quote == '\''))) {
      throw syntax_error(quote, context);
    }
    buffer.clear();
    for (;;) {
      int ch = istream.get();
      if (ch == quote) {
        break;
      } else if (ch < ' ') {
        throw syntax_error(ch, context);
      } else if (ch == '\\') {
        // [escape]
        ch = istream.get();
        switch (ch) {
        case '\'':
          if (!has_flag(flags::single_quote)) {
            throw syntax_error(ch, context);
          }
          break;
        case '"':
        case '\\':
        case '/':
          break;
        case 'b':
          ch = '\b';
          break;
        case 'f':
          ch = '\f';
          break;
        case 'n':
          ch = '\n';
          break;
        case 'r':
          ch = '\r';
          break;
        case 't':
          ch = '\t';
          break;
        case 'u':
          // ['u' hex hex hex hex]
          {
            char16_t code = 0;
            for (int i = 0; i < 4; ++i) {
              ch = istream.get();
              int n = to_number_hex(ch);
              if (n < 0) {
                throw syntax_error(ch, context);
              }
              code = static_cast<char16_t>((code << 4) + n);
            }
            if (code < 0x80) {
              buffer.append(1, (char)code);
            } else if (code < 0x800) {
              buffer.append(1, (char)(0xc0 | (code >> 6)));
              buffer.append(1, (char)(0x80 | (code & 0x3f)));
            } else {
              buffer.append(1, (char)(0xe0 | (code >> 12)));
              buffer.append(1, (char)(0x80 | ((code >> 6) & 0x3f)));
              buffer.append(1, (char)(0x80 | (code & 0x3f)));
            }
          }
          continue;
        case '\r':
          if (has_flag(flags::multi_line_string)) {
            ch = istream.get();
            if (ch != '\n') {
              istream.unget();
            }
            continue;
          }
          /* no-break */
        case '\n':
          if (has_flag(flags::multi_line_string)) {
            continue;
          }
          /* no-break */
        default:
          throw syntax_error(ch, context);
        }
      }
      buffer.append(1, (char)ch);
    }
  }

  /**
   * @brief Parse string value
   *
   * @param v A value object to store parsed value
   * @param quote The first quote character
   */
  void parse_string(value& v, int quote)
  {
    static const char context[] = "string";
    v = "";
    parse_string(v.as_string(), quote, context);
  }

  /**
   * @brief Parse array value
   *
   * @param v A value object to store parsed value
   */
  void parse_array(value& v)
  {
    static const char context[] = "array";
    v = array({});
    auto& elements = v.as_array();
    for (;;) {
      int ch = skip_spaces();
      if (ch == ']') {
        break;
      }
      if (elements.empty()) {
        istream.unget();
      } else if (ch != ',') {
        throw syntax_error(ch, context);
      } else if (has_flag(trailing_comma)) {
        ch = skip_spaces();
        if (ch == ']') {
          break;
        }
        istream.unget();
      }
      // [value]
      elements.emplace_back(nullptr);
      parse_value(elements.back(), context);
    }
  }

  /**
   * @brief Parse object key
   *
   * @return A parsed string
   */
  std::string parse_key()
  {
    static const char context[] = "object-key";
    std::string buffer;
    int ch = skip_spaces();
    if (has_flag(flags::unquoted_key)) {
      if ((ch != '"') && (ch != '\'')) {
        for (;; ch = istream.get()) {
          if ((ch == '_') || (ch == '$') || (is_alpha(ch))) {
            // [IdentifierStart]
          } else if (is_digit(ch) && (!buffer.empty())) {
            // [UnicodeDigit]
          } else if (ch == ':') {
            break;
          } else {
            throw syntax_error(ch, context);
          }
          buffer.append(1, (char)ch);
        }
        istream.unget();
        return buffer;
      }
    }
    parse_string(buffer, ch, context);
    return buffer;
  }

  /**
   * @brief Parse object value
   *
   * @param v A value object to store parsed value
   */
  void parse_object(value& v)
  {
    static const char context[] = "object";
    v = object({});
    auto& elements = v.as_object();
    for (;;) {
      int ch = skip_spaces();
      if (ch == '}') {
        break;
      }
      if (elements.empty()) {
        istream.unget();
      } else if (ch != ',') {
        throw syntax_error(ch, context);
      } else if (has_flag(flags::trailing_comma)) {
        ch = skip_spaces();
        if (ch == '}') {
          break;
        }
        istream.unget();
      }
      // [string]
      // [key] (JSON5)
      const std::string key = parse_key();
      ch = skip_spaces();
      if (ch != ':') {
        throw syntax_error(ch, context);
      }
      // [value]
      auto result = elements.emplace(key, nullptr);
      parse_value(result.first->second, context);
    }
  }

  std::istream& istream;  ///< An input stream
};

/**
 * @brief Stringifier implementation
 *
 * @tparam F A combination of flags
 * @tparam I An indent specification
 */
template <flags_type F, indent_type I>
class stringifier
{
private:
  using self_type = stringifier<F,I>;
  static constexpr auto M = flags::stringify_mask;

public:
  /**
   * @brief Construct a new stringifier object
   *
   * @param ostream An output stream
   */
  stringifier(std::ostream& ostream) : ostream(ostream) {}

  /**
   * @brief Apply flag manipulator
   *
   * @tparam S Flags to be set
   * @tparam C Flags to be cleared
   * @param manip A manipulator
   * @return An updated stringifier object
   */
  template <flags_type S, flags_type C>
  stringifier<((F&~C)|S)&M,I> operator<<(const manipulator_flags<S,C>& manip)
  {
    return stringifier<((F&~C)|S)&M,I>(ostream);
  }

  /**
   * @brief Apply indent manipulator
   *
   * @tparam NI A new indent specification
   * @param manip A manipulator
   * @return stringifier<F,NI>
   * @return An updated stringifier object
   */
  template <indent_type NI>
  stringifier<F,NI> operator<<(const manipulator_indent<NI>& manip)
  {
    return stringifier<F,NI>(ostream);
  }

  /**
   * @brief Delegate manipulator to std::ostream
   *
   * @param manip A stream manipulator
   * @return An output stream
   */
  std::ostream& operator<<(std::ostream& (*manip)(std::ostream&))
  {
    return ostream << manip;
  }

  /**
   * @brief Delegate operator<< to std::ostream
   *
   * @tparam T A typename of argument
   * @param v A value
   * @return An output stream
   */
  template <class T>
  std::ostream& operator<<(const T& v)
  {
    return ostream << v;
  }

  /**
   * @brief Stringify JSON
   *
   * @param v A value object to stringify
   * @return A reference to self
   */
  self_type& operator<<(const value& v)
  {
    do_stringify(v);
    return *this;
  }

private:
  /**
   * @brief Check if flag(s) enabled
   *
   * @param flags Combination of flags to be tested
   * @retval true Any flag is enabled
   * @retval False No flag is enabled
   */
  static constexpr bool has_flag(flags_type flags)
  {
    return (F & flags) != 0;
  }

  /**
   * @brief Get newline code
   *
   * @return A newline string literal
   */
  static const char *get_newline()
  {
    return (F & flags::crlf_newline) ? "\r\n" : "\n";
  }

  /**
   * @brief Get the indent text
   *
   * @return An indent text for one level
   */
  static value::json_type get_indent()
  {
    if (I > 0) {
      return value::json_type(I, ' ');
    } else if (I < 0) {
      return value::json_type(-I, '\t');
    }
    return value::json_type();
  }

  /**
   * @brief Stringifier entry
   *
   * @param v A value object to stringify
   * @param indent An indent string
   */
  void do_stringify(const value& v)
  {
    class fmtsaver
    {
    public:
      fmtsaver(std::ios_base& ios)
      : ios(ios), flags(ios.flags()), width(ios.width())
      {
      }
      ~fmtsaver()
      {
        ios.width(width);
        ios.flags(flags);
      }
    private:
      std::ios_base& ios;
      const std::ios_base::fmtflags flags;
      const std::streamsize width;
    };
    fmtsaver saver(ostream);
    stringify_value(v, "");
  }

  /**
   * @brief Stringify value
   *
   * @param v A value object to stringify
   * @param indent An indent string
   */
  void stringify_value(const value& v, const value::json_type& indent)
  {
    switch (v.type) {
    case value::TYPE_BOOLEAN:
      ostream << (v.content.boolean ? "true" : "false");
      break;
    case value::TYPE_NUMBER:
      if (std::isnan(v.content.number)) {
        if (!has_flag(flags::not_a_number)) {
          goto null;
        }
        ostream << "NaN";
      } else if (!std::isfinite(v.content.number)) {
        if (!has_flag(flags::infinity_number)) {
          goto null;
        }
        ostream << ((v.content.number > 0) ? "infinity" : "-infinity");
      } else {
        ostream << v.content.number;
      }
      break;
    case value::TYPE_INTEGER:
      ostream << v.content.integer;
      break;
    case value::TYPE_STRING:
      stringify_string(v.content.string);
      break;
    case value::TYPE_ARRAY:
      if (v.content.array.empty()) {
        ostream << "[]";
      } else if (I == 0) {
        const char *delim = "[";
        for (const auto& item : v.content.array) {
          ostream << delim;
          stringify_value(item, indent);
          delim = ",";
        }
        ostream << "]";
      } else {
        const char *const newline = get_newline();
        const char *delim = "[";
        const value::json_type inner_indent = indent + get_indent();
        for (const auto& item : v.content.array) {
          ostream << delim << newline << inner_indent;
          stringify_value(item, inner_indent);
          delim = ",";
        }
        ostream << newline << indent << "]";
      }
      break;
    case value::TYPE_OBJECT:
      if (v.content.object.empty()) {
        ostream << "{}";
      } else if (I == 0) {
        const char *delim = "{";
        for (const auto& pair : v.content.object) {
          ostream << delim;
          stringify_string(pair.first);
          ostream << ":";
          stringify_value(pair.second, indent);
          delim = ",";
        }
        ostream << "}";
      } else {
        const char *const newline = get_newline();
        const char *delim = "{";
        const value::json_type inner_indent = indent + get_indent();
        for (const auto& pair : v.content.object) {
          ostream << delim << newline << inner_indent;
          stringify_string(pair.first);
          ostream << ": ";
          stringify_value(pair.second, inner_indent);
          delim = ",";
        }
        ostream << newline << indent << "}";
      }
      break;
    default:
    null:
      ostream << "null";
      break;
    }
  }

  /**
   * @brief Stringify string
   *
   * @param string A string to be stringified
   */
  void stringify_string(const value::string_type& string)
  {
    ostream << "\"";
    for (const auto& i : string) {
      const auto ch = (unsigned char)i;
      static const char hex[] = "0123456789abcdef";
      switch (ch) {
      case '"':  ostream << "\\\""; break;
      case '\\': ostream << "\\\\"; break;
      case '\b': ostream << "\\b";  break;
      case '\f': ostream << "\\f";  break;
      case '\n': ostream << "\\n";  break;
      case '\r': ostream << "\\r";  break;
      case '\t': ostream << "\\t";  break;
      default:
        if (ch < ' ') {
          ostream << "\\u00";
          ostream.put(hex[(ch >> 4) & 0xf]);
          ostream.put(hex[ch & 0xf]);
        } else {
          ostream.put(ch);
        }
        break;
      }
    }
    ostream << "\"";
  }

  std::ostream& ostream;  ///< An output stream
};

/**
 * @brief Apply flag manipulator to std::istream
 *
 * @param istream An input stream
 * @param manip A flag manipulator
 * @return A new parser
 */
template <flags_type S, flags_type C>
parser<S&flags::parse_mask> operator>>(std::istream& istream, const manipulator_flags<S,C>& manip)
{
  return parser<S&flags::parse_mask>(istream) >> manip;
}

/**
 * @brief Apply flag manipulator to std::ostream
 *
 * @param ostream An output stream
 * @param manip A flag manipulator
 * @return A new stringifier
 */
template <flags_type S, flags_type C>
stringifier<S&flags::stringify_mask,0> operator<<(std::ostream& ostream, const manipulator_flags<S,C>& manip)
{
  return stringifier<S&flags::stringify_mask,0>(ostream) << manip;
}

/**
 * @brief Apply indent manipulator to std::istream
 *
 * @param istream An input stream
 * @param manip An indent manipulator
 * @return A new parser
 */
template <indent_type I>
parser<0> operator>>(std::istream& istream, const manipulator_indent<I>& manip)
{
  return parser<0>(istream) >> manip;
}

/**
 * @brief Apply indent manipulator to std::ostream
 *
 * @param ostream An output stream
 * @param manip An indent manipulator
 * @return A new stringifier
 */
template <indent_type I>
stringifier<0,I> operator<<(std::ostream& ostream, const manipulator_indent<I>& manip)
{
  return stringifier<0,0>(ostream) << manip;
}

/**
 * @brief Flow manipulator/value into stringifier
 *
 * @tparam S A typename of stringifier
 * @tparam T A typename of manipulator / value
 * @tparam Args A list of typenames of other manipulators / values
 * @param stringifier A stringifier
 * @param value A manipulator / value
 * @param args Other manipulators / values
 */
template <class S, class T, class... Args>
static void flow_stringifier(S stringifier, T& value, Args&... args)
{
  flow_stringifier(stringifier << value, args...);
}

/**
 * @brief Recursive expansion stopper for flow_stringifier
 *
 * @tparam S A typename of stringifier
 * @param stringifier A stringifier
 */
template <class S>
static void flow_stringifier(S& stringifier)
{
}

class membuf : public std::streambuf
{
public:
  explicit membuf(const void* data, std::size_t size)
  {
    const auto p = reinterpret_cast<char*>(const_cast<void*>(data));
    setg(p, p, p + size);
  }
};

class imemstream : public std::istream
{
public:
  explicit imemstream(const void* data, std::size_t size)
  : std::istream(&buf), buf(data, size) {}
private:
  membuf buf;
};

} /* namespace impl */

/**
 * @brief Parse JSON from an input stream (with ECMA-404 standard rule)
 *
 * @param istream An input stream
 * @param v A value to store parsed value
 * @return A new parser
 */
inline impl::parser<0> operator>>(std::istream& istream, value& v)
{
  return impl::parser<0>(istream) >> v;
}

/**
 * @brief Stringify JSON to an output stream (with ECMA-404 standard rule)
 *
 * @param ostream An output stream
 * @param v A value to stringify
 * @return A new stringifier
 */
inline impl::stringifier<0,0> operator<<(std::ostream& ostream, const value& v)
{
  return impl::stringifier<0,0>(ostream) << v;
}

namespace rule {

/**
 * @brief Allow single line comment starts with "//"
 */
using single_line_comment       = impl::manipulator_flags<impl::flags::single_line_comment, 0>;

/**
 * @brief Disallow single line comment starts with "//"
 */
using no_single_line_comment    = impl::manipulator_flags<0, impl::flags::single_line_comment>;

/**
 * @brief Allow multi line comment starts with "/ *" and ends with "* /"
 */
using multi_line_comment        = impl::manipulator_flags<impl::flags::multi_line_comment, 0>;

/**
 * @brief Disallow multi line comment starts with "/ *" and ends with "* /"
 */
using no_multi_line_comment     = impl::manipulator_flags<0, impl::flags::multi_line_comment>;

/**
 * @brief Allow any comments
 */
using comments                  = impl::manipulator_flags<impl::flags::comments, 0>;

/**
 * @brief Disallow any comments
 */
using no_comments               = impl::manipulator_flags<0, impl::flags::comments>;

/**
 * @brief Allow explicit plus sign(+) before non-negative number
 */
using explicit_plus_sign        = impl::manipulator_flags<impl::flags::explicit_plus_sign, 0>;

/**
 * @brief Disallow explicit plus sign(+) before non-negative number
 */
using no_explicit_plus_sign     = impl::manipulator_flags<0, impl::flags::explicit_plus_sign>;

/**
 * @brief Allow leading decimal point before number
 */
using leading_decimal_point     = impl::manipulator_flags<impl::flags::leading_decimal_point, 0>;

/**
 * @brief Disallow leading decimal point before number
 */
using no_leading_decimal_point  = impl::manipulator_flags<0, impl::flags::leading_decimal_point>;

/**
 * @brief Allow trailing decimal point after number
 */
using trailing_decimal_point    = impl::manipulator_flags<impl::flags::trailing_decimal_point, 0>;

/**
 * @brief Disallow trailing decimal point after number
 */
using no_trailing_decimal_point = impl::manipulator_flags<0, impl::flags::trailing_decimal_point>;

/**
 * @brief Allow leading/trailing decimal points beside number
 */
using decimal_points            = impl::manipulator_flags<impl::flags::decimal_points, 0>;

/**
 * @brief Disallow leading/trailing decimal points beside number
 */
using no_decimal_points         = impl::manipulator_flags<0, impl::flags::decimal_points>;

/**
 * @brief Allow infinity (infinity/-infinity) as number value
 */
using infinity_number           = impl::manipulator_flags<impl::flags::infinity_number, 0>;

/**
 * @brief Disallow infinity (infinity/-infinity) as number value
 */
using no_infinity_number        = impl::manipulator_flags<0, impl::flags::infinity_number>;

/**
 * @brief Allow NaN as number value
 */
using not_a_number              = impl::manipulator_flags<impl::flags::not_a_number, 0>;

/**
 * @brief Disallow NaN as number value
 */
using no_not_a_number           = impl::manipulator_flags<0, impl::flags::not_a_number>;

/**
 * @brief Allow hexadeciaml number
 */
using hexadecimal               = impl::manipulator_flags<impl::flags::hexadecimal, 0>;

/**
 * @brief Disallow hexadeciaml number
 */
using no_hexadecimal            = impl::manipulator_flags<0, impl::flags::hexadecimal>;

/**
 * @brief Allow single-quoted string
 */
using single_quote              = impl::manipulator_flags<impl::flags::single_quote, 0>;

/**
 * @brief Disallow single-quoted string
 */
using no_single_quote           = impl::manipulator_flags<0, impl::flags::single_quote>;

/**
 * @brief Allow multi line string escaped by "\"
 */
using multi_line_string         = impl::manipulator_flags<impl::flags::multi_line_string, 0>;

/**
 * @brief Disallow multi line string escaped by "\"
 */
using no_multi_line_string      = impl::manipulator_flags<0, impl::flags::multi_line_string>;

/**
 * @brief Allow trailing comma in arrays and objects
 */
using trailing_comma            = impl::manipulator_flags<impl::flags::trailing_comma, 0>;

/**
 * @brief Disallow trailing comma in arrays and objects
 */
using no_trailing_comma         = impl::manipulator_flags<0, impl::flags::trailing_comma>;

/**
 * @brief Allow unquoted keys in objects
 */
using unquoted_key              = impl::manipulator_flags<impl::flags::unquoted_key, 0>;

/**
 * @brief Disallow unquoted keys in objects
 */
using no_unquoted_key           = impl::manipulator_flags<0, impl::flags::unquoted_key>;

/**
 * @brief Set ECMA-404 standard rules
 * @see https://www.json.org/
 */
using ecma404                   = impl::manipulator_flags<0, impl::flags::all_rules>;

/**
 * @brief Set JSON5 rules
 * @see https://json5.org/
 */
using json5                     = impl::manipulator_flags<impl::flags::json5_rules, 0>;

/**
 * @brief Parse as finished(closed) JSON
 */
using finished                  = impl::manipulator_flags<impl::flags::finished, 0>;

/**
 * @brief Parse as streaming(non-closed) JSON
 */
using streaming                 = impl::manipulator_flags<0, impl::flags::finished>;

/**
 * @brief Use LF as newline code (when indent enabled)
 */
using lf_newline                = impl::manipulator_flags<0, impl::flags::crlf_newline>;

/**
 * @brief Use CR+LF as newline code (when indent enabled)
 */
using crlf_newline              = impl::manipulator_flags<impl::flags::crlf_newline, 0>;

/**
 * @brief Disable indent
 */
using no_indent                 = impl::manipulator_indent<0>;

/**
 * @brief Enable indent with tab "\t" character
 */
template <impl::indent_type I = 1>
using tab_indent                = impl::manipulator_indent<static_cast<impl::indent_type>(-I)>;

/**
 * @brief Enable indent with space " " character
 */
template <impl::indent_type I = 2>
using space_indent              = impl::manipulator_indent<I>;

} /* namespace rule */

/**
 * @brief Parse string as JSON (ECMA-404 standard)
 *
 * @param istream An input stream
 * @param finished If true, parse as finished(closed) JSON
 * @return JSON value
 */
inline value parse(std::istream& istream, bool finished = true)
{
  using namespace impl;
  value v;
  if (finished) {
    parser<flags::finished>(istream) >> v;
  } else {
    parser<0>(istream) >> v;
  }
  return v;
}

/**
 * @brief Parse string as JSON (ECMA-404 standard)
 *
 * @param string A string to be parsed
 * @return JSON value
 */
inline value parse(const value::json_type& string)
{
  std::istringstream istream(string);
  return parse(istream, true);
}

/**
 * @brief Parse string as JSON (ECMA-404 standard)
 *
 * @param pointer A pointer to string to be parsed
 * @param length Length of string (in bytes)
 * @return JSON value
 */
inline value parse(const void* pointer, std::size_t length)
{
  impl::imemstream istream(pointer, length);
  return parse(istream, true);
}

/**
 * @brief Parse string as JSON (JSON5)
 *
 * @param istream An input stream
 * @param finished If true, parse as finished(closed) JSON
 * @return JSON value
 */
inline value parse5(std::istream& istream, bool finished = true)
{
  using namespace impl;
  value v;
  if (finished) {
    parser<flags::json5_rules|flags::finished>(istream) >> v;
  } else {
    parser<flags::json5_rules>(istream) >> v;
  }
  return v;
}

/**
 * @brief Parse string as JSON (JSON5)
 *
 * @param string A string to be parsed
 * @return JSON value
 */
inline value parse5(const value::json_type& string)
{
  std::istringstream istream(string);
  return parse5(istream, true);
}

/**
 * @brief Parse string as JSON (JSON5)
 *
 * @param pointer A pointer to string to be parsed
 * @param length Length of string (in bytes)
 * @return JSON value
 */
inline value parse5(const void* pointer, std::size_t length)
{
  impl::imemstream istream(pointer, length);
  return parse5(istream, true);
}

/**
 * @brief Stringify value (ECMA-404 standard)
 *
 * @tparam T A list of typenames of manipulators
 * @param v A value to stringify
 * @param args A list of manipulators
 * @return JSON string
 */
template <class... T>
value::json_type stringify(const value& v, T... args)
{
  std::ostringstream ostream;
  impl::flow_stringifier(ostream << rule::ecma404(), args..., v);
  return ostream.str();
}

/**
 * @brief Stringify value (JSON5)
 *
 * @tparam T A list of typenames of manipulators
 * @param v A value to stringify
 * @param args A list of manipulators
 * @return JSON string
 */
template <class... T>
value::json_type stringify5(const value& v, T... args)
{
  std::ostringstream ostream;
  impl::flow_stringifier(ostream << rule::json5(), args..., v);
  return ostream.str();
}

/**
 * @brief Stringify value (ECMA-404 standard)
 *
 * @tparam T A list of typenames of manipulators
 * @param args A list of manipulators
 * @return JSON string
 */
template <class... T>
value::json_type value::stringify(T... args) const
{
  return json5pp::stringify(*this, args...);
}

/**
 * @brief Stringify value (JSON5)
 *
 * @tparam T A list of typenames of manipulators
 * @param args A list of manipulators
 * @return JSON string
 */
template <class... T>
value::json_type value::stringify5(T... args) const
{
  return json5pp::stringify5(*this, args...);
}

} /* namespace json5pp */

#endif  /* _JSON5PP_HPP_ */
