#pragma once

#include <memory>

namespace lcc
{
    class Codegen
    {
    public:
        ~Codegen() = default;
        Codegen(const Codegen &) = delete;
        Codegen &operator=(const Codegen &) = delete;

    private:
        static std::unique_ptr<Codegen> _inst;

        static bool EnableDebugBuffering;

        Codegen();

    public:
        static Codegen *getInstance()
        {
            if (_inst.get() == nullptr)
                _inst.reset(new Codegen);

            return _inst.get();
        }

        int run();

    private:
        static int compileModule(char ** args, llvm::LLVMContext & context);
    };
}