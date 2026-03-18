#include "Attribute.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <cassert>
#include <atomic>
#include "mylog.h"
#include <string>

struct MyStruct {
	int x;
	double y;
	bool operator==(const MyStruct& other) const { return x == other.x && y == other.y; }
};

void test_string() {
	mylog(MyLogLevel::I, "[Test] String...");
	Attribute attr(std::string("hello"));
	assert(attr.get<std::string>() == "hello");
	attr.set<std::string>("world");
	assert(attr.get<std::string>() == "world");
	mylog(MyLogLevel::I, "OK");
}

void test_bool() {
	mylog(MyLogLevel::I, "[Test] Bool...");
	Attribute attr(true);
	assert(attr.get<bool>() == true);
	attr.set<bool>(false);
	assert(attr.get<bool>() == false);
	mylog(MyLogLevel::I, "OK");
}

void test_struct() {
	mylog(MyLogLevel::I, "[Test] Struct...");
	MyStruct s1{1, 2.5}, s2{3, 4.5};
	Attribute attr(s1);
	assert(attr.get<MyStruct>() == s1);
	attr.set<MyStruct>(s2);
	assert(attr.get<MyStruct>() == s2);
	mylog(MyLogLevel::I, "OK");
}

void test_basic() {
	mylog(MyLogLevel::I, "[Test] Basic set/get... ");
	Attribute attr(42);
	assert(attr.get<int>() == 42);
	attr.set<int>(100);
	assert(attr.get<int>() == 100);
	mylog(MyLogLevel::I, "OK");
}

void test_callback() {
		mylog(MyLogLevel::I, "[Test] Callback... ");
	Attribute attr(1);
	int callback_count = 0;
	attr.addHandler([&](const std::any& oldValue, const std::any& newValue) {
		++callback_count;
		assert(std::any_cast<int>(oldValue) == 1);
		assert(std::any_cast<int>(newValue) == 2);
	});
	attr.set<int>(2);
	assert(callback_count == 1);
		mylog(MyLogLevel::I, "OK");
}

void test_type_safety() {
		mylog(MyLogLevel::I, "[Test] Type safety... ");
	Attribute attr(3.14);
	try {
		attr.get<int>();
		assert(false && "Should throw bad_any_cast");
	} catch (const std::bad_any_cast& e) {
		mylog(MyLogLevel::E, "Exception in test_type_safety get: ", e.what());
	}
	try {
		attr.set<int>(5); // type mismatch
		assert(false && "Should throw bad_any_cast");
	} catch (const std::bad_any_cast& e) {
		mylog(MyLogLevel::E, "Exception in test_type_safety set: ", e.what());
	}
		mylog(MyLogLevel::I, "OK");
}

void test_concurrent() {
		mylog(MyLogLevel::I, "[Test] Concurrent set/get... ");
	Attribute attr(0);
	constexpr int N = 8;
	constexpr int rounds = 10000;
	std::atomic<int> sum{0};
	std::vector<std::thread> threads;
	// Writer threads
	for (int i = 0; i < N; ++i) {
		threads.emplace_back([&, i]() {
			for (int j = 0; j < rounds; ++j) {
				attr.set<int>(i * rounds + j);
			}
		});
	}
	// Reader threads
	for (int i = 0; i < N; ++i) {
		threads.emplace_back([&]() {
			for (int j = 0; j < rounds; ++j) {
				try {
					sum += attr.get<int>();
				} catch (const std::exception& e) {
					mylog(MyLogLevel::E, "Exception in test_concurrent: ", e.what());
				} catch (...) {
					mylog(MyLogLevel::E, "Unknown exception in test_concurrent");
				}
			}
		});
	}
	for (auto& t : threads) t.join();
		mylog(MyLogLevel::I, "OK");
}

void test_remove_handler() {
		mylog(MyLogLevel::I, "[Test] Remove handler... ");
	Attribute attr(10);
	int called = 0;
	Attribute::ValueChangedCallback cb = [&](const std::any&, const std::any&) { ++called; };
	attr.addHandler(cb);
	attr.set<int>(20);
	attr.removeHandler(cb);
	attr.set<int>(30);
	assert(called == 1);
    mylog(MyLogLevel::I, "OK");
}

void test_multithread_stress() {
	mylog(MyLogLevel::I, "[Test] Multi-thread stress...");
	Attribute attr(0);
	constexpr int N = 16;
	constexpr int rounds = 20000;
	std::atomic<int> int_sum{0};
	std::atomic<int> bool_true_count{0};
	std::atomic<int> string_ok_count{0};
	std::vector<std::thread> threads;
	// Mix of writers and readers, with different types
	for (int i = 0; i < N; ++i) {
		threads.emplace_back([&, i]() {
			for (int j = 0; j < rounds; ++j) {
				int op = (i + j) % 3;
				try {
					if (op == 0) {
						attr.set<int>(i * rounds + j);
					} else if (op == 1) {
						attr.set<bool>((j % 2) == 0);
					} else {
						attr.set<std::string>(j % 2 == 0 ? "ok" : "fail");
					}
				} catch (const std::exception& e) {
					mylog(MyLogLevel::E, "Exception in stress set: ", e.what());
				}
			}
		});
	}
	for (int i = 0; i < N; ++i) {
		threads.emplace_back([&, i]() {
			for (int j = 0; j < rounds; ++j) {
				try {
					int op = (i + j) % 3;
					if (op == 0) {
						int_sum += attr.get<int>();
					} else if (op == 1) {
						if (attr.get<bool>()) bool_true_count++;
					} else {
						if (attr.get<std::string>() == "ok") string_ok_count++;
					}
				} catch (const std::exception& e) {
					mylog(MyLogLevel::E, "Exception in stress get: ", e.what());
				}
			}
		});
	}
	for (auto& t : threads) t.join();
	mylog(MyLogLevel::I, "Multi-thread stress finished. int_sum=", int_sum.load(), ", bool_true_count=", bool_true_count.load(), ", string_ok_count=", string_ok_count.load());
}

int main() {
	test_basic();
	test_callback();
	test_type_safety();
	test_concurrent();
	test_remove_handler();
	test_string();
	test_bool();
	test_struct();
	// test_multithread_stress();
	mylog(MyLogLevel::I, "All tests passed!");
	return 0;
}
