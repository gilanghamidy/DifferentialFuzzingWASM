#include "jsapi.h"
#include "jsapi-ext.h"
#include "js/Initialization.h"
#include "js/CompilationAndEvaluation.h"
#include "js/SourceText.h"
#include "js/Conversions.h"
#include "js/Modules.h"
#include <iostream>
#include <fstream>
#include <memory>

#include "runner-common.h"

/* The class of the global object. */
static JSClass global_class = {
    "global",
    JSCLASS_GLOBAL_FLAGS,
    &JS::DefaultGlobalClassOps
};

int main(int argc, const char *argv[])
{
  if(argc < 2) {
    std::cerr << "Need file input argument" << std::endl;
    return 1;
  }


  JS_Init();

  JSContext *cx = JS_NewContext(8L * 1024 * 1024);
  if (!cx)
      return 1;
  if (!JS::InitSelfHostedCode(cx))
      return 1;

  { // Scope for our various stack objects (JSAutoRequest, RootedObject), so they all go
    // out of scope before we JS_DestroyContext.

    JS::RealmOptions options;
    JS::RootedObject global(cx, JS_NewGlobalObject(cx, &global_class, nullptr, JS::FireOnNewGlobalHook, options));
    if (!global)
        return 1;

    JS::RootedValue rval(cx);

    { // Scope for JSAutoCompartment
      JSAutoRealm ac(cx, global);
      JS::InitRealmStandardClasses(cx);



      auto inputInstruction = dfw::OpenInput(argv[1]);
      auto res = js::ext::CompileWasmBytes(cx, inputInstruction.data(), inputInstruction.size());
      
      dfw::DumperLooper([&] (auto arg) { 
        auto func = (*res)[arg];
        return func 
                ? std::make_optional(func->instructions) 
                : std::nullopt;
      });

      /*
      std::vector<JS::Value> args;
      
      args.emplace_back(JS::Int32Value(25));
      args.emplace_back();
      args.emplace_back(js::ext::CreateBigIntValue(cx, 150ll));
      args.emplace_back(js::ext::CreateBigIntValue(cx, 300ll));
      //std::cout << "Param 1: " << args[2].toInt32() << std::endl;
      bool invokeRes = (*res)["subi64"].Invoke(cx, args);

      std::cout << "Invoke result: " << invokeRes << std::endl;
      JS::Value& val = args[0];*/
      //std::cout << "Func result: " << js::ext::GetBigIntValue(val) /*.toInt32()*/ << std::endl;
      

      /*
      for(auto& func : res->functions()) {
        std::cout << "Function Name: " << func.name << "\nInstructions: \n";
        DumpDisassemble(std::cout, func.instructions);
      }*/
    }
  }

  JS_DestroyContext(cx);
  JS_ShutDown();
  return 0;
}