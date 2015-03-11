#pragma once
#include <fc/exception/exception.hpp>

#include <include/v8.h>
#include <include/libplatform/libplatform.h>

namespace bts { namespace game {
   
   using namespace v8;
   
   class v8_helper
   {
   public:
      // Creates a new execution environment containing the built-in
      // functions.
      static v8::Handle<v8::Context> CreateShellContext(v8::Isolate* isolate);
      
      // The callback that is invoked by v8 whenever the JavaScript 'print'
      // function is called.  Prints its arguments on stdout separated by
      // spaces and ending with a newline.
      static void Print(const v8::FunctionCallbackInfo<v8::Value>& args);
      
      
      // The callback that is invoked by v8 whenever the JavaScript 'read'
      // function is called.  This function loads the content of the file named in
      // the argument into a JavaScript string.
      static void Read(const v8::FunctionCallbackInfo<v8::Value>& args);
      
      
      // The callback that is invoked by v8 whenever the JavaScript 'load'
      // function is called.  Loads, compiles and executes its argument
      // JavaScript file.
      static void Load(const v8::FunctionCallbackInfo<v8::Value>& args);
      
      
      // The callback that is invoked by v8 whenever the JavaScript 'quit'
      // function is called.  Quits.
      static void Quit(const v8::FunctionCallbackInfo<v8::Value>& args);
      
      
      static void Version(const v8::FunctionCallbackInfo<v8::Value>& args);
      
      // Reads a file into a v8 string.
      static v8::Handle<v8::String> ReadFile(v8::Isolate* isolate, const char* name);
      
      static bool ExecuteString(v8::Isolate* isolate, v8::Handle<v8::String> source, v8::Handle<v8::String> name, bool print_result, bool report_exceptions);
      
      static void ReportException(v8::Isolate* isolate, v8::TryCatch* try_catch);
      
      static const char* ToCString(const v8::String::Utf8Value& value);
   };
}}