#include <AST.h>

[[nodiscard]] extern Ref<Type> FindVariable(const std::string& name);

Ref<Type> VariableExpressionAST::GetType() const 
{
    return FindVariable(name);
}

Ref<Type> CallExpressionAST::GetType() const 
{
    return FindVariable(calleeName);
}
