//
// Created by liyinbin on 2020/2/2.
//


#include <gtest/gtest.h>
#include <set>
#include <unordered_map>
#include <abel/asl/string_view.h>
#include <abel/memory/shared_ptr.h>

using namespace abel;

struct expected_exception : public std::exception {
};

struct A {
    static bool destroyed;

    A() {
        destroyed = false;
    }

    virtual ~A() {
        destroyed = true;
    }
};

struct A_esft : public A, public enable_lw_shared_from_this<A_esft> {
};

struct B {
    virtual void x() {}
};

bool A::destroyed = false;

TEST(shared_ptr, explot_dynamic_cast_use_after_free_problem) {
    shared_ptr<A> p = ::make_shared<A>();
    {
        auto p2 = dynamic_pointer_cast<B>(p);
        ABEL_ASSERT(!p2);
    }
    ABEL_ASSERT(!A::destroyed);
}

class C : public enable_shared_from_this<C> {
public:
    shared_ptr<C> dup() { return shared_from_this(); }

    shared_ptr<const C> get() const { return shared_from_this(); }
};

TEST(shared_ptr, test_const_ptr) {
    shared_ptr<C> a = make_shared<C>();
    shared_ptr<const C> ca = a;
    EXPECT_TRUE(ca == a);
    shared_ptr<const C> cca = ca->get();
    EXPECT_TRUE(cca == ca);
}
/*
struct D {
};

TEST(shared_ptr, test_lw_const_ptr_1) {
    auto pd1 = make_lw_shared<const D>(D());
    auto pd2 = make_lw_shared(D());
    lw_shared_ptr<const D> pd3 = pd2;
    EXPECT_TRUE(pd2 == pd3);
}

struct E : enable_lw_shared_from_this<E> {
};

TEST(shared_ptr, test_lw_const_ptr_2) {
    auto pe1 = make_lw_shared<const E>();
    auto pe2 = make_lw_shared<E>();
    lw_shared_ptr<const E> pe3 = pe2;
    EXPECT_TRUE(pe2 == pe3);
}

struct F : enable_lw_shared_from_this<F> {
    lw_shared_ptr<const F> const_method() const {
        return shared_from_this();
    }
};

TEST(shared_ptr, test_shared_from_this_called_on_const_object) {
    auto ptr = make_lw_shared<F>();
    ptr->const_method();
}

TEST(shared_ptr, test_exception_thrown_from_constructor_is_propagated) {
    struct X {
        X() {
            throw expected_exception();
        }
    };
    try {
        auto ptr = make_lw_shared<X>();
        EXPECT_TRUE(1) << "onstructor should have thrown";
    } catch (const expected_exception &e) {
        EXPECT_TRUE(1) << "Expected exception caught";
    }
    try {
        auto ptr = ::make_shared<X>();
        EXPECT_TRUE(1) << "Constructor should have thrown";
    } catch (const expected_exception &e) {
        EXPECT_TRUE(1) << "Expected exception caught";
    }
}

TEST(shared_ptr, test_indirect_functors) {
    {
        std::multiset<shared_ptr<string_view>, indirect_less<shared_ptr<string_view>>> a_set;

        a_set.insert(make_shared<string_view>("k3"));
        a_set.insert(make_shared<string_view>("k1"));
        a_set.insert(make_shared<string_view>("k2"));
        a_set.insert(make_shared<string_view>("k4"));
        a_set.insert(make_shared<string_view>("k0"));


        auto i = a_set.begin();
        EXPECT_EQ(string_view("k0"), *(*i++));
        EXPECT_EQ(string_view("k1"), *(*i++));
        EXPECT_EQ(string_view("k2"), *(*i++));
        EXPECT_EQ(string_view("k3"), *(*i++));
        EXPECT_EQ(string_view("k4"), *(*i++));
    }

    {
        std::unordered_map<shared_ptr<string_view>, bool,
                indirect_hash<shared_ptr<string_view>>, indirect_equal_to<shared_ptr<string_view>>> a_map;

        a_map.emplace(make_shared<string_view>("k3"), true);
        a_map.emplace(make_shared<string_view>("k1"), true);
        a_map.emplace(make_shared<string_view>("k2"), true);
        a_map.emplace(make_shared<string_view>("k4"), true);
        a_map.emplace(make_shared<string_view>("k0"), true);

        EXPECT_TRUE(a_map.count(make_shared<string_view>("k0")));
        EXPECT_TRUE(a_map.count(make_shared<string_view>("k1")));
        EXPECT_TRUE(a_map.count(make_shared<string_view>("k2")));
        EXPECT_TRUE(a_map.count(make_shared<string_view>("k3")));
        EXPECT_TRUE(a_map.count(make_shared<string_view>("k4")));
        EXPECT_TRUE(!a_map.count(make_shared<string_view>("k5")));
    }
}

template<typename T>
void do_test_release() {
    auto ptr = make_lw_shared<T>();
    EXPECT_TRUE(!T::destroyed);

    auto ptr2 = ptr;

    EXPECT_TRUE(!ptr.release());
    EXPECT_TRUE(!ptr);
    EXPECT_TRUE(ptr2.use_count() == 1);

    auto uptr2 = ptr2.release();
    EXPECT_TRUE(uptr2);
    EXPECT_TRUE(!ptr2);
    ptr2 = {};

    EXPECT_TRUE(!T::destroyed);
    uptr2 = {};

    EXPECT_TRUE(T::destroyed);

    // Check destroying via disposer
    auto ptr3 = make_lw_shared<T>();
    auto uptr3 = ptr3.release();
    EXPECT_TRUE(uptr3);
    EXPECT_TRUE(!T::destroyed);

    auto raw_ptr3 = uptr3.release();
    lw_shared_ptr<T>::dispose(raw_ptr3);
    EXPECT_TRUE(T::destroyed);
}

TEST(shared_ptr, test_release) {
    do_test_release<A>();
    do_test_release<A_esft>();
}

TEST(shared_ptr, test_const_release) {
    do_test_release<const A>();
    do_test_release<const A_esft>();
}
*/