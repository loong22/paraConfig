#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <variant>
#include <memory>
#include <type_traits> // For SFINAE tools

// --- 模拟您的引擎类 (保持不变) ---
struct EngineA {
    void Initialize(int x, const std::string& y) {
        std::cout << "EngineA::Initialize(int, string) called with: " << x << ", " << y << std::endl;
    }
};

struct EngineB {
    void Initialize() {
        std::cout << "EngineB::Initialize() called." << std::endl;
    }
};

struct EngineC {
    void Start() {}
};

// --- 定义 Variant 类型 (保持不变) ---
using EngineVariant = std::variant<
    std::unique_ptr<EngineA>,
    std::unique_ptr<EngineB>,
    std::unique_ptr<EngineC>
>;

// --- 正确的 C++17 SFINAE 解决方案 ---
namespace detail {
    // 默认实现，继承自 false_type
    template <typename, typename T, typename... Args>
    struct has_initialize_impl : std::false_type {};

    // 特化版本。仅当 std::void_t<> 中的表达式有效时，此特化才会被选择
    template <typename T, typename... Args>
    struct has_initialize_impl<
        std::void_t<decltype(std::declval<T&>().Initialize(std::declval<Args>()...))>,
        T,
        Args...
    > : std::true_type {
    };
} // namespace detail

// 公开的接口，它使用 detail::has_initialize_impl 来完成工作
template <typename T, typename... Args>
struct has_initialize_with_args : detail::has_initialize_impl<void, T, Args...> {};

// 辅助变量模板，方便使用
template<typename T, typename... Args>
inline constexpr bool has_initialize_with_args_v = has_initialize_with_args<T, Args...>::value;


// --- InitializeSubEngines 函数 (保持不变) ---
template<typename EngineVariantType, typename... Args>
void InitializeSubEngines(
    const std::vector<std::string>& executionOrder,
    std::map<std::string, EngineVariantType>& subEnginesPool,
    Args&&... args)
{
    for (const auto& subEngineName : executionOrder) {
        if (subEnginesPool.count(subEngineName)) {
            std::visit([&](auto&& engine_ptr) {
                if (engine_ptr) {
                    using SmartPtrType = std::decay_t<decltype(engine_ptr)>;
                    using T = typename SmartPtrType::element_type;

                    if constexpr (has_initialize_with_args_v<T, Args...>) {
                        engine_ptr->Initialize(std::forward<Args>(args)...);
                    }
                    else if constexpr (has_initialize_with_args_v<T>) {
                        engine_ptr->Initialize();
                    }
                    else {
                        std::cout << "Engine of type " << typeid(T).name() << " has no matching Initialize method." << std::endl;
                    }
                }
                }, subEnginesPool.at(subEngineName));
        }
    }
}

// --- main 函数 (保持不变) ---
int main() {
    std::map<std::string, EngineVariant> subEnginesPool;
    subEnginesPool["A"] = std::make_unique<EngineA>();
    subEnginesPool["B"] = std::make_unique<EngineB>();
    subEnginesPool["C"] = std::make_unique<EngineC>();

    std::vector<std::string> order = { "A", "B", "C" };

    std::cout << "--- Calling with (int, string) arguments ---" << std::endl;
    InitializeSubEngines(order, subEnginesPool, 100, std::string("hello"));

    std::cout << "\n--- Calling with no arguments ---" << std::endl;
    InitializeSubEngines(order, subEnginesPool);

    return 0;
}
