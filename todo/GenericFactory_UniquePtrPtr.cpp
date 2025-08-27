#include <iostream>
#include <string>
#include <map>
#include <functional>
#include <memory>
#include <stdexcept>
#include <variant>
#include <utility> // For std::move
#include <type_traits>

// 为清晰起见，使用一个简单的资源类
class Resource {
public:
    Resource(const std::string& n) : name_(n) {
        std::cout << "Resource '" << name_ << "' created.\n";
    }
    ~Resource() {
        std::cout << "Resource '" << name_ << "' DESTROYED.\n";
    }
    void use() {
        std::cout << "Using Resource '" << name_ << "'.\n";
    }
private:
    std::string name_;
};

// --- 使用 Unique Pointer 的对象池，实现借出/归还模式 ---
template <typename VariantType>
class UniqueObjectPool {
private:
    using FactoryFunction = std::function<VariantType(const std::string&)>;

    std::map<std::string, FactoryFunction> factories_;
    // variant 现在持有 unique_ptr
    std::map<std::string, VariantType> pool_;

public:
    // 注册类型及其创建方式
    template <typename T>
    void registerType(const std::string& typeName) {
        static_assert(std::is_constructible_v<VariantType, std::unique_ptr<T>>,
                      "The provided type T is not a valid alternative for the VariantType.");

        factories_[typeName] = [](const std::string& instanceName) -> VariantType {
            return std::make_unique<T>(instanceName);
        };
    }

    // 根据类型名创建实例并放入池中
    void create(const std::string& typeName, const std::string& instanceName) {
        if (pool_.count(instanceName)) {
            throw std::runtime_error("Object with name '" + instanceName + "' already exists.");
        }
        auto factory_it = factories_.find(typeName);
        if (factory_it == factories_.end()) {
            throw std::runtime_error("Unknown type: " + typeName);
        }
        // 使用工厂函数创建实例并移入池中
        pool_.emplace(instanceName, factory_it->second(instanceName));
    }
    
    // "借出"对象，将所有权转移给调用者
    template <typename T>
    std::unique_ptr<T> checkout(const std::string& name) {
        auto it = pool_.find(name);
        if (it == pool_.end()) {
            std::cout << "Pool: No object named '" << name << "' found.\n";
            return nullptr;
        }

        // 尝试从 variant 中获取 unique_ptr 的指针
        if (auto* ptr_to_unique_ptr = std::get_if<std::unique_ptr<T>>(&it->second)) {
            // 关键：移出指针。池中的 unique_ptr 在此之后变为空。
            // 所有权被转移给调用者。
            if (*ptr_to_unique_ptr) {
                std::cout << "Pool: Checking out '" << name << "'.\n";
                return std::move(*ptr_to_unique_ptr);
            }
        }

        std::cout << "Pool: Object '" << name << "' is of a different type or is already checked out.\n";
        return nullptr; // 类型错误或指针已为空
    }

    // "归还"对象，将所有权移回池中
    template <typename T>
    void checkin(const std::string& name, std::unique_ptr<T> object_ptr) {
        if (!object_ptr) {
            std::cout << "Pool: Cannot check in a null pointer for '" << name << "'.\n";
            return;
        }

        auto it = pool_.find(name);
        if (it == pool_.end()) {
            throw std::runtime_error("Cannot check in object '" + name + "': no slot exists.");
        }

        if (auto* ptr_to_unique_ptr = std::get_if<std::unique_ptr<T>>(&it->second)) {
            if (*ptr_to_unique_ptr) {
                // 如果池中指针不为空，说明归还逻辑有误（比如重复归还）
                throw std::runtime_error("Pool: Slot for '" + name + "' is already occupied. Cannot check in.");
            }
            // 将对象所有权移回池中
            std::cout << "Pool: Checking in '" << name << "'.\n";
            *ptr_to_unique_ptr = std::move(object_ptr);
        } else {
            throw std::runtime_error("Pool: Slot for '" + name + "' is of a different type.");
        }
    }
};

int main() {
    using MyResourceVariant = std::variant<std::unique_ptr<Resource>>;
    UniqueObjectPool<MyResourceVariant> pool;

    // 注册 Resource 类型
    pool.registerType<Resource>("Resource");

    std::cout << "--- 1. Create resource in the pool ---\n";
    pool.create("Resource", "MyPrecious");

    std::cout << "\n--- 2. Main function 'checks out' the resource ---\n";
    std::unique_ptr<Resource> my_res_ptr = pool.checkout<Resource>("MyPrecious");

    if (my_res_ptr) {
        std::cout << "Main: Successfully checked out, now using it.\n";
        my_res_ptr->use();
    }

    std::cout << "\n--- 3. Main function tries to check out again (should fail) ---\n";
    auto another_ptr = pool.checkout<Resource>("MyPrecious");
    if (!another_ptr) {
        std::cout << "Main: As expected, cannot check out again because it's already in use.\n";
    }

    std::cout << "\n--- 4. Main function 'checks in' the resource to the pool ---\n";
    pool.checkin("MyPrecious", std::move(my_res_ptr));

    // 归还后, my_res_ptr 变为空指针，不能再使用
    if (!my_res_ptr) {
        std::cout << "Main: my_res_ptr is now nullptr, ownership has been returned to the pool.\n";
    }

    std::cout << "\n--- 5. Another part of the program checks out the resource again ---\n";
    auto final_ptr = pool.checkout<Resource>("MyPrecious");
    if (final_ptr) {
        std::cout << "Main: Successfully checked out the resource for the second time.\n";
        final_ptr->use();
    }
    
    std::cout << "\n--- 6. Program ends. final_ptr goes out of scope, destroying the resource ---\n";
    // 当 main 结束时, final_ptr 被销毁, 从而调用 Resource 的析构函数。
    // 如果我们最后把它归还了，那么池的析构函数会负责销毁它。
    return 0;
}
