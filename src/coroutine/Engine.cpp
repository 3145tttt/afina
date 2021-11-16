#include <afina/coroutine/Engine.h>

#include <setjmp.h>
#include <stdio.h>
#include <string.h>

namespace Afina {
namespace Coroutine {

void Engine::Store(context &ctx) { //save stack
    char a;
    ctx.Hight = &a;

    uint32_t need_size = ctx.Low - ctx.Hight;
    delete [] std::get<0>(ctx.Stack);
    std::get<0>(ctx.Stack) = new char[need_size];
    std::get<1>(ctx.Stack) = need_size;
    memcpy(std::get<0>(ctx.Stack), ctx.Hight, need_size);
}

void Engine::Restore(context &ctx) { //restore stack and jmp
    char a;
    if (&a > ctx.Hight && &a < ctx.Low) {
        Restore(ctx);
    }
    memcpy(ctx.Hight, std::get<0>(ctx.Stack), std::get<1>(ctx.Stack));
    cur_routine = &ctx;
    longjmp(ctx.Environment, 1);
}

void Engine::yield() { //choose new task
    context *cur_ctx = alive;
    if (cur_ctx == cur_routine && cur_ctx != nullptr) {
        cur_ctx = cur_ctx->next;
    }
    if (cur_ctx != nullptr) {
        sched(cur_ctx);
    }
}

void Engine::sched(void *routine_) {
    context *cur = (context *) routine_;
    if (cur_routine != idle_ctx && setjmp(cur_routine->Environment) > 0) {
        return;  
    }
    Store(*cur_routine); //save cur
    cur_routine = cur; //change
    Restore(*cur_routine); //restore
}

} // namespace Coroutine
} // namespace Afina
