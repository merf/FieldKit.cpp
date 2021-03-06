/*                                                                           
 *      _____  __  _____  __     ____                                   
 *     / ___/ / / /____/ / /    /    \   FieldKit
 *    / ___/ /_/ /____/ / /__  /  /  /   (c) 2010, FIELD. All rights reserved.              
 *   /_/        /____/ /____/ /_____/    http://www.field.io           
 *   
 *	 Created by Marcus Wendt on 15/11/2010.
 */

#include "fieldkit/script/ScriptContext.h"
#include "fieldkit/script/Module.h"
#include "fieldkit/Logger.h"
#include <boost/foreach.hpp>
#include <boost/filesystem/operations.hpp>
#include <sstream>
#include <stdexcept>

using namespace fieldkit::script;


// -- Helpers ------------------------------------------------------------------

void ReportException(TryCatch* tryCatch)
{
    //	HandleScope handleScope;
    
	String::Utf8Value exception(tryCatch->Exception());
	const char* exceptionString = ToCString(exception);
	Handle<Message> message = tryCatch->Message();
    
	if (message.IsEmpty()) {
		// V8 didn't provide any extra information about this error; just
		// print the exception.
		throw  std::runtime_error("Unknown error "+ std::string(exceptionString));
        
	} else {
		// Print (filename):(line number): (message).
		String::Utf8Value filename(message->GetScriptResourceName());
		const char* filenameString = ToCString(filename);
		int linenum = message->GetLineNumber();
        
        std::stringstream ss;
        
        // Print filename and linenum if applicable
        if(!message->GetScriptResourceName()->IsUndefined()) {
            ss << filenameString <<":"<< linenum <<" ";
        }
        
		ss << exceptionString << std::endl;
        
		// Print line of source code.
		String::Utf8Value sourceline(message->GetSourceLine());
		const char* sourceline_string = ToCString(sourceline);
        
		ss << sourceline_string << std::endl;
        
		// Print wavy underline (GetUnderline is deprecated).
		int start = message->GetStartColumn();
		for (int i = 0; i < start; i++) {
			ss << " ";
		}
		int end = message->GetEndColumn();
		for (int i = start; i < end; i++) {
			ss << "^";
		}
		ss << std::endl;
        
		String::Utf8Value stack_trace(tryCatch->StackTrace());
		if (stack_trace.length() > 0) {
			const char* stack_trace_string = ToCString(stack_trace);
			ss << stack_trace_string;
		}
        
        throw std::runtime_error(ss.str());
	}
}


// Executes a string within the current v8 context.
bool ExecuteString(Handle<String> source) 
{
	TryCatch tryCatch;
    
	Handle<Script> script = Script::Compile(source);
    
	if (script.IsEmpty()) {
        ReportException(&tryCatch);
		return false;
        
	} else {
		Handle<Value> result = script->Run();
		if (result.IsEmpty()) {
            ReportException(&tryCatch);
			return false;
            
		} else {
//			if(!result->IsUndefined()) {
//				// If all went well and the result wasn't undefined then print
//				// the returned value.
//				String::Utf8Value str(result);
//				const char* cstr = ToCString(str);
//				printf("%s\n", cstr);
//                delete cstr;
//			}
			return true;
		}
	}
}


// Reads a file into a stl string.
static string ReadFileContents(string const& path)
{
    FILE* file = fopen(path.c_str(), "rb");
    if (file == NULL) return "";
    
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
    
    string contents = string(chars, size);
    delete chars;
    
    return contents;
}


// Parses a given source string for '#include "filename"' macros and replaces these with the actual file contents
static void ResolveIncludes(string const& path, string& source)
{
    string key = "#include";
    
    size_t match = source.find(key);
    if(match == string::npos) 
        return;
    
    size_t start = source.find('"', match);
    size_t end = source.find('"', start+1);
    size_t len = end - start -1;
    
    string file = source.substr(start+1, len);
    string content = ReadFileContents(path +"/" + file);
    
    if(content == "") 
        content = " print(\"ERROR: Couldnt include file '"+ file +"'\")";
    
    source.replace(match, end - match +1, content);
    
    ResolveIncludes(path, source);
}


static std::time_t GetNewestFileWriteTime(string const& _path)
{
    namespace fs = boost::filesystem;
    
    std::size_t newestWrite = 0;
    
    // check if any of the script files have changed
    if(!fs::exists(_path)) return newestWrite;
    
    fs::directory_iterator endIt;
    for(fs::directory_iterator it(_path); it != endIt; ++it) {
        std::time_t fileWrite = fs::last_write_time(it->path());
        if(fileWrite > newestWrite) {
            newestWrite = fileWrite;
        }
    }
    return newestWrite;
}


// -- Core Bindings ------------------------------------------------------------
static string currentScriptPath = "";

v8::Handle<Value> Require(Arguments const& args) 
{	
    if(args.Length() != 1) return Boolean::New(false);
    
    try {
        HandleScope handleScope;
        
        String::Utf8Value filePathUTF(args[0]);

        std::stringstream ss;
        ss << currentScriptPath << "/" << string(*filePathUTF, filePathUTF.length());
        string filePath = ss.str();
        
        string fileContents = ReadFileContents(filePath.c_str());
        
        bool success = false;
        
        // execute script
        if(fileContents != "") {
            Handle<String> source = String::New(fileContents.c_str());
            success = ExecuteString(source);
        }
        
        return Boolean::New(success);
        
    } catch(std::exception& e) {
        LOG_ERROR(e.what());
        return Boolean::New(false);
    }
}


// -- Script Context -----------------------------------------------------------
ScriptContext::~ScriptContext()
{
    filePath = "";
    parentPath = "";
    
    context.Dispose();
    
	BOOST_FOREACH(Module* m, modules) {
		delete m;
	}
	modules.clear();
}

void ScriptContext::add(Module* module)
{
	modules.push_back(module);
}

bool ScriptContext::execute(std::string sourceOrFile) 
{
    // check if arguments are valid
    if(sourceOrFile == "" && filePath == "") {
        throw std::runtime_error("Calling execute without arguments requires to set a filepath before!");
        return false;
    }
    
    
    // check if argument is a path to a script file
    std::string source = "";
    
    namespace fs = boost::filesystem;
    if(fs::exists(sourceOrFile)) {
        this->filePath = sourceOrFile;
        this->parentPath = boost::filesystem::path(filePath).parent_path().string();
        
    // if not and its not empty - its probably a javascript source string
    } else if(sourceOrFile != "") {
        this->filePath = "";
        this->parentPath = "";
        source = sourceOrFile;
    }
    
    // when filePath is set - load source from file
    if(filePath != "") {
//        LOG_INFO("loading script from path "<< parentPath);
        
        // set global script path
        currentScriptPath = parentPath;
        
        // load main script file and resolve includes
        source = ReadFileContents(filePath);
        ResolveIncludes(parentPath, source);
        
        // remember newest write time to see if any file has changed later
        newestWriteTime = GetNewestFileWriteTime(parentPath);
        
        if(source == "") {
            throw std::runtime_error("Error reading '"+ filePath +"'");
            return false;
        }
    }
    
    // make sure v8 is still active
	if(V8::IsDead())
		return false;
    
    // clean up old execution environment if set
    if(Context::InContext()) {
        context->Exit();
        context.Dispose();
    }
    
	HandleScope handleScope;
    
	// create a template for the global object.
	Handle<ObjectTemplate> global = ObjectTemplate::New();

	// create a new execution environment containing the attached functions
    // TODO Context::New seems to be leaking memory, 
    context = Context::New(NULL, global);
    
	// enter the newly created execution environment.
	Context::Scope contextScope(context);
    
    // register core and additional bindings
    attachBindings();
    
	// execute script
    Handle<String> _source = String::New(source.c_str());
    return ExecuteString(_source);
}

void ScriptContext::attachBindings()
{
    SET_METHOD(context->Global(), "require", Require);
    BOOST_FOREACH(Module* m, modules) {
        m->Initialize(context->Global());
    }   
}

bool ScriptContext::filesModified()
{
    // no parent path set yet
    if(parentPath == "") return false;
    
    std::time_t writeTime = GetNewestFileWriteTime(parentPath);
    if(writeTime > newestWriteTime) {
//        LOG_INFO("Script file modification detected");
        newestWriteTime = writeTime;
        return true;
        
    } else {
        return false;
    }
}

Handle<Object> ScriptContext::newInstance(Handle<Object> localContext, Handle<String> name, int argc, Handle<Value>* argv) 
{
    HandleScope handleScope;
    TryCatch tryCatch;
    
    Handle<Value> value = localContext->Get(name);
    Handle<Function> func = Handle<Function>::Cast(value);
    Handle<Value> result = func->NewInstance(argc, argv);
    
    if(result.IsEmpty())
        ReportException(&tryCatch);
    
    return handleScope.Close(Handle<Object>::Cast(result));
}

Handle<Value> ScriptContext::call(Handle<Object> localContext, const char* name, int argc, Handle<Value>* argv)
{
    return call(localContext, String::New(name), argc, argv);
}

Handle<Value> ScriptContext::call(Handle<Object> localContext, Handle<String> name, int argc, Handle<Value>* argv) 
{
    HandleScope handleScope;
    TryCatch tryCatch;
    
    Handle<Value> value = localContext->Get(name);
    
    // check if function exists within given context
    if(!value->IsUndefined()) {
        Handle<Function> func = Handle<Function>::Cast(value);
        Handle<Value> result = func->Call(localContext, argc, argv);
        
        if(result.IsEmpty())
            ReportException(&tryCatch);
        
        return handleScope.Close(result);
    }
    
    return Undefined();
}
