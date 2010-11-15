/*                                                                           
 *      _____  __  _____  __     ____                                   
 *     / ___/ / / /____/ / /    /    \   FieldKit
 *    / ___/ /_/ /____/ / /__  /  /  /   (c) 2010, FIELD. All rights reserved.              
 *   /_/        /____/ /____/ /_____/    http://www.field.io           
 *   
 *	 Created by Marcus Wendt on 15/11/2010.
 */

#include "fieldkit/script/Script.h"
#include "fieldkit/script/Binding.h"

using namespace fieldkit::script;

Script::~Script()
{
	clear();
}

void Script::add(Binding* binding)
{
	bindings.push_back(binding);
}

void Script::clear()
{
	BOOST_FOREACH(Binding* b, bindings) {
		delete b;
	}
	bindings.clear();
}

void Script::run(std::string _file) 
{
	LOG_INFO("Script::run '"<< _file <<"'");
	
	HandleScope handleScope;

	// -- Create Context --
	
	// Create a template for the global object.
	ObjectTemplatePtr global = ObjectTemplate::New();
	
	BOOST_FOREACH(Binding* b, bindings) {
		b->prepare(global);
	}
	
	// Create a new execution environment containing the built-in functions
	ContextPtr context = Context::New(NULL, global);
	
	// Enter the newly created execution environment.
	Context::Scope context_scope(context);
	
	
	// -- Expose Script Objects --
	BOOST_FOREACH(Binding* b, bindings) {
		b->init(context);
	}
	
	
	// -- Execute Script --
	
	//HandleScope handle_scope;
	StringPtr fileName = String::New(_file.c_str());
	StringPtr source = readFile(_file.c_str());
	if (source.IsEmpty()) {
        printf("Error reading '%s'\n", _file.c_str());
        //return 1;
	}
	if (!executeString(source, fileName, false, true)) {
        printf("Error executing script \n");
       //return 1;
	}

	
	// -- Cleanup --
	v8::V8::Dispose();
}


// -- Helpers ------------------------------------------------------------------

// Reads a file into a v8 string.
StringPtr Script::readFile(std::string path) {
	FILE* file = fopen(path.c_str(), "rb");
	if (file == NULL) return StringPtr();
	
	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	rewind(file);
	
	char* chars = new char[size + 1];
	chars[size] = '\0';
	for (int i = 0; i < size;) {
		int read = fread(&chars[i], 1, size - i, file);
		i += read;
	}
	fclose(file);
	StringPtr result = String::New(chars, size);
	delete[] chars;
	return result;
}



// Executes a string within the current v8 context.
bool Script::executeString(StringPtr source, ValuePtr name,
						   bool print_result, bool report_exceptions) {
	HandleScope handle_scope;
	v8::TryCatch try_catch;
	v8::Handle<v8::Script> script = v8::Script::Compile(source, name);
	if (script.IsEmpty()) {
		// Print errors that happened during compilation.
		if (report_exceptions)
			reportException(&try_catch);
		return false;
	} else {
		ValuePtr result = script->Run();
		if (result.IsEmpty()) {
			// Print errors that happened during execution.
			if (report_exceptions)
				reportException(&try_catch);
			return false;
		} else {
			if (print_result && !result->IsUndefined()) {
				// If all went well and the result wasn't undefined then print
				// the returned value.
				String::Utf8Value str(result);
				const char* cstr = ToCString(str);
				printf("%s\n", cstr);
			}
			return true;
		}
	}
}

void Script::reportException(v8::TryCatch* try_catch)
{
	HandleScope handle_scope;
	String::Utf8Value exception(try_catch->Exception());
	const char* exception_string = ToCString(exception);
	v8::Handle<v8::Message> message = try_catch->Message();
	if (message.IsEmpty()) {
		// V8 didn't provide any extra information about this error; just
		// print the exception.
		printf("%s\n", exception_string);
	} else {
		// Print (filename):(line number): (message).
		String::Utf8Value filename(message->GetScriptResourceName());
		const char* filename_string = ToCString(filename);
		int linenum = message->GetLineNumber();
		printf("%s:%i: %s\n", filename_string, linenum, exception_string);
		// Print line of source code.
		String::Utf8Value sourceline(message->GetSourceLine());
		const char* sourceline_string = ToCString(sourceline);
		printf("%s\n", sourceline_string);
		// Print wavy underline (GetUnderline is deprecated).
		int start = message->GetStartColumn();
		for (int i = 0; i < start; i++) {
			printf(" ");
		}
		int end = message->GetEndColumn();
		for (int i = start; i < end; i++) {
			printf("^");
		}
		printf("\n");
		String::Utf8Value stack_trace(try_catch->StackTrace());
		if (stack_trace.length() > 0) {
			const char* stack_trace_string = ToCString(stack_trace);
			printf("%s\n", stack_trace_string);
		}
	}
}


// -- Functions ----------------------------------------------------------------

namespace fieldkit { namespace script {
	
	// Extracts a C string from a V8 Utf8Value.
	const char* ToCString(const String::Utf8Value& value) 
	{
		return *value ? *value : "<string conversion failed>";
	}

	// Converts a v8::String to a std::string
	const std::string ToStdString(StringPtr handle)
	{
		return std::string( *String::AsciiValue(handle) );
	}

} } // fieldkit::script
