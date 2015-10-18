#ifndef JASON_PARSER_H
#define JASON_PARSER_H 1

#include <math.h>

#include "JasonAsm.h"
#include "JasonType.h"
#include "Jason.h"
#include "JasonBuilder.h"

namespace arangodb {
  namespace jason {

    class JasonParser {

      struct ParsedNumber {
        ParsedNumber ()
          : intValue(0),
            doubleValue(0.0),
            isInteger(true) {
        }

        void addDigit (int i) {
          if (isInteger) {
            // check if adding another digit to the int will make it overflow
            if (intValue < 1844674407370955161ULL ||
                (intValue == 1844674407370955161ULL && (i - '0') <= 5)) {
              // int won't overflow
              intValue = intValue * 10 + (i - '0');
              return;
            }
            // int would overflow
            doubleValue = static_cast<double>(intValue);
            isInteger = false;
          }

          doubleValue = doubleValue * 10.0 + (i - '0');
          if (std::isnan(doubleValue) || ! std::isfinite(doubleValue)) {
            throw JasonParserError("numeric value out of bounds");
          }
        }

        double asDouble () const {
          if (isInteger) {
            return static_cast<double>(intValue);
          }
          return doubleValue;
        }

        uint64_t intValue;
        double doubleValue;
        bool isInteger;
      };

      // This class can parse JSON very rapidly, but only from contiguous
      // blocks of memory. It builds the result using the JasonBuilder.
      //
      // Use as follows:
      //   JasonParser p;
      //   std::string json = "{\"a\":12}";
      //   try {
      //     size_t nr = p.parse(json);
      //   }
      //   catch (std::bad_alloc e) {
      //     std::cout << "Out of memory!" << std::endl;
      //   }
      //   catch (JasonParserError e) {
      //     std::cout << "Parse error: " << e.what() << std::endl;
      //     std::cout << "Position of error: " << p.errorPos() << std::endl;
      //   }
      //   JasonBuilder b = p.steal();
      //
      //   // p is now empty again and ready to parse more.

        JasonBuilder   _b;
        uint8_t const* _start;
        size_t         _size;
        size_t         _pos;

      public:

        struct JasonParserError : std::exception {
          private:
            std::string _msg;
          public:
            JasonParserError (std::string const& msg) : _msg(msg) {
            }
            char const* what() const noexcept {
              return _msg.c_str();
            }
        };

        JasonOptions options;        

        JasonParser (JasonParser const&) = delete;
        JasonParser& operator= (JasonParser const&) = delete;

        JasonParser () : _start(nullptr), _size(0), _pos(0) {
        }

        JasonLength parse (std::string const& json, bool multi = false) {
          _start = reinterpret_cast<uint8_t const*>(json.c_str());
          _size  = json.size();
          _pos   = 0;
          _b.clear();
          return parseInternal(multi);
        }

        JasonLength parse (uint8_t const* start, size_t size,
                           bool multi = false) {
          _start = start;
          _size = size;
          _pos = 0;
          _b.clear();
          return parseInternal(multi);
        }

        JasonLength parse (char const* start, size_t size,
                           bool multi = false) {
          _start = reinterpret_cast<uint8_t const*>(start);
          _size = size;
          _pos = 0;
          _b.clear();
          return parseInternal(multi);
        }

        // We probably want a parse from stream at some stage...
        // Not with this high-performance two-pass approach. :-(
        
        JasonBuilder&& steal () {
          return std::move(_b);
        }

        // Beware, only valid as long as you do not parse more, use steal
        // to move the data out!
        uint8_t const* jason () {
          return _b.start();
        }

        // Returns the position at the time when the just reported error
        // occurred, only use when handling an exception.
        size_t errorPos () const {
          return _pos > 0 ? _pos - 1 : _pos;
        }

        void clear () {
          _b.clear();
        }

      private:

        inline int peek () const {
          if (_pos >= _size) {
            return -1;
          }
          return static_cast<int>(_start[_pos]);
        }

        inline int consume () {
          if (_pos >= _size) {
            return -1;
          }
          return static_cast<int>(_start[_pos++]);
        }

        inline void unconsume () {
          --_pos;
        }

        inline void reset () {
          _pos = 0;
        }

        JasonLength parseInternal (bool multi);

        inline bool isWhiteSpace (uint8_t i) const noexcept {
          return (i == ' ' || i == '\t' || i == '\n' || i == '\r');
        }

        // skips over all following whitespace tokens but does not consume the
        // byte following the whitespace
        int skipWhiteSpace (char const* err) {
          if (_pos >= _size) {
            throw JasonParserError(err);
          }
          uint8_t c = _start[_pos];
          if (! isWhiteSpace(c)) {
            return c;
          }
          if (c == ' ') {
            if (_pos+1 >= _size) {
              _pos++;
              throw JasonParserError(err);
            }
            c = _start[_pos+1];
            if (! isWhiteSpace(c)) {
              _pos++;
              return c;
            }
          }
          size_t remaining = _size - _pos;
          size_t count = JSONSkipWhiteSpace(_start + _pos, remaining);
          _pos += count;
          if (count < remaining) {
            return static_cast<int>(_start[_pos]);
          }
          throw JasonParserError(err);
        }

        void parseTrue () {
          // Called, when main mode has just seen a 't', need to see "rue" next
          if (consume() != 'r' || consume() != 'u' || consume() != 'e') {
            throw JasonParserError("true expected");
          }
          _b.addTrue();
        }

        void parseFalse () {
          // Called, when main mode has just seen a 'f', need to see "alse" next
          if (consume() != 'a' || consume() != 'l' || consume() != 's' ||
              consume() != 'e') {
            throw JasonParserError("false expected");
          }
          _b.addFalse();
        }

        void parseNull () {
          // Called, when main mode has just seen a 'n', need to see "ull" next
          if (consume() != 'u' || consume() != 'l' || consume() != 'l') {
            throw JasonParserError("null expected");
          }
          _b.addNull();
        }
        
        void scanDigits (ParsedNumber& value) {
          while (true) {
            int i = consume();
            if (i < 0) {
              return;
            }
            if (i < '0' || i > '9') {
              unconsume();
              return;
            }
            value.addDigit(i);
          }
        }

        double scanDigitsFractional () {
          double pot = 0.1;
          double x = 0.0;
          while (true) {
            int i = consume();
            if (i < 0) {
              return x;
            }
            if (i < '0' || i > '9') {
              unconsume();
              return x;
            }
            x = x + pot * (i - '0');
            pot /= 10.0;
          }
        }

        inline int getOneOrThrow (char const* msg) {
          int i = consume();
          if (i < 0) {
            throw JasonParserError(msg);
          }
          return i;
        }

        void parseNumber ();

        void parseString ();

        void parseArray ();

        void parseObject ();

        void parseJson ();

    };

  }  // namespace arangodb::jason
}  // namespace arangodb

#endif
