#include <stddef.h>
#include <unistd.h>

#include <string>

struct my_pair {
    int i;
    void *v;
};

template <typename T>
struct my_type
{
    T templated;
    std::string obj_name;
    void *v;
};

int
main()
{
    struct my_pair p;
    p.i = 42;
    p.v = nullptr;
    my_type<my_pair> my_type_my_pair { { 2020, nullptr }, "MY_PAIR", nullptr };
    my_type<int> my_type_int { {}, "INT", nullptr };
    my_type_my_pair.v = &my_type_my_pair.templated;
    my_type_int.v = &my_type_int.templated;
    pause();
    return 0;
}
