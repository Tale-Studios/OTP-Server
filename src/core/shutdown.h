#pragma once
#include <exception>

// otp_handle_signals sets up signal handlers for the native OS
void otp_handle_signals();

// otp_shutdown tells otp to exit gracefully with a given error code
void otp_shutdown(int exit_code, bool throw_exception = true);

// ShutdownException is thrown by otp_shutdown to prevent
// the current thread from continuing execution.
class ShutdownException : public std::exception
{
  private:
    int m_exit_code;

  public:
    ShutdownException(int exit_code);
    int exit_code() const;
    virtual const char* what() const throw();
};
