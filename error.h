#pragma once

#include <stdexcept>
#include <string>

#include <cxxabi.h>
#include <errno.h>
#include <execinfo.h>
#include <signal.h>

namespace buzzdb {

enum class ExceptionType {
  INVALID_EXCEPTION = 0,  // invalid type

  NOT_IMPLEMENTED_EXCEPTION = 1,
  SCHEMA_PARSING_EXCEPTION = 2,

};

class Exception : public std::runtime_error {
 public:
  Exception(ExceptionType exception_type)
      : std::runtime_error(""), type(exception_type) {
    exception_message_ =
        "Exception Type :: " + ExceptionTypeToString(exception_type);
  }

  Exception(std::string message)
      : std::runtime_error(message), type(ExceptionType::INVALID_EXCEPTION) {
    exception_message_ = "Message :: " + message;
  }

  Exception(ExceptionType exception_type, std::string message)
      : std::runtime_error(message), type(exception_type) {
    exception_message_ =
        "Exception Type :: " + ExceptionTypeToString(exception_type) +
        "\nMessage :: " + message;
  }

  std::string GetMessage() { return exception_message_; }

  std::string ExceptionTypeToString(ExceptionType type) {
    switch (type) {
      case ExceptionType::INVALID_EXCEPTION:
        return "Invalid";
      case ExceptionType::NOT_IMPLEMENTED_EXCEPTION:
        return "Not Implemented";
      case ExceptionType::SCHEMA_PARSING_EXCEPTION:
        return "Schema Parsing";

      default:
        return "Unknown";
    }
  }

  // Based on :: http://panthema.net/2008/0901-stacktrace-demangled/
  static void PrintStackTrace(FILE *out = ::stderr,
                              unsigned int max_frames = 63) {
    ::fprintf(out, "Stack Trace:\n");

    /// storage array for stack trace address data
    void *addrlist[max_frames + 1];

    /// retrieve current stack addresses
    int addrlen = backtrace(addrlist, max_frames + 1);

    if (addrlen == 0) {
      ::fprintf(out, "  <empty, possibly corrupt>\n");
      return;
    }

    /// resolve addresses into strings containing "filename(function+address)",
    char **symbol_list = backtrace_symbols(addrlist, addrlen);

    /// allocate string which will be filled with the demangled function name
    size_t func_name_size = 1024;
    std::unique_ptr<char> func_name(new char[func_name_size]);

    /// iterate over the returned symbol lines. skip the first, it is the
    /// address of this function.
    for (int i = 1; i < addrlen; i++) {
      char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

      /// find parentheses and +address offset surrounding the mangled name:
      /// ./module(function+0x15c) [0x8048a6d]
      for (char *p = symbol_list[i]; *p; ++p) {
        if (*p == '(')
          begin_name = p;
        else if (*p == '+')
          begin_offset = p;
        else if (*p == ')' && begin_offset) {
          end_offset = p;
          break;
        }
      }

      if (begin_name && begin_offset && end_offset &&
          begin_name < begin_offset) {
        *begin_name++ = '\0';
        *begin_offset++ = '\0';
        *end_offset = '\0';

        /// mangled name is now in [begin_name, begin_offset) and caller
        /// offset in [begin_offset, end_offset). now apply  __cxa_demangle():
        int status;
        char *ret = abi::__cxa_demangle(begin_name, func_name.get(),
                                        &func_name_size, &status);
        if (status == 0) {
          func_name.reset(ret);  // use possibly realloc()-ed string
          ::fprintf(out, "  %s : %s+%s\n", symbol_list[i], func_name.get(),
                    begin_offset);
        } else {
          /// demangling failed. Output function name as a C function with
          /// no arguments.
          ::fprintf(out, "  %s : %s()+%s\n", symbol_list[i], begin_name,
                    begin_offset);
        }
      } else {
        /// couldn't parse the line ? print the whole line.
        ::fprintf(out, "  %s\n", symbol_list[i]);
      }
    }
  }

  friend std::ostream &operator<<(std::ostream &os, const Exception &e);

 private:
  // type
  ExceptionType type;
  std::string exception_message_;
};

//===--------------------------------------------------------------------===//
// Derived classes
//===--------------------------------------------------------------------===//

class NotImplementedException : public Exception {
 public:
  NotImplementedException()
      : Exception(ExceptionType::NOT_IMPLEMENTED_EXCEPTION) {}
};

class SchemaParseException : public Exception {
  SchemaParseException() = delete;

 public:
  SchemaParseException(std::string msg)
      : Exception(ExceptionType::SCHEMA_PARSING_EXCEPTION, msg) {}
};

}  
