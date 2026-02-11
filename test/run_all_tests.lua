-- 总测试入口 - 运行所有测试用例
-- Add build folder to cpath for loading .so files
package.cpath = package.cpath .. ";../build/?.so"

print("="..string.rep("=", 60))
print("Running All Sparse Set Tests")
print("="..string.rep("=", 60))
print()

-- 加载测试模块
local test1 = dofile("test/test1.lua")
local test2 = require("test/test2")

local total_tests = 0
local passed_tests = 0
local failed_tests = 0

-- 执行单个测试的辅助函数
local function run_test(name, test_func)
    total_tests = total_tests + 1
    print(string.format("[%d/%d] Running: %s", total_tests, total_tests, name))
    print(string.rep("-", 60))
    
    local success, err = pcall(test_func)
    
    if success then
        passed_tests = passed_tests + 1
        print("✓ PASSED")
    else
        failed_tests = failed_tests + 1
        print("✗ FAILED")
        print("Error: " .. tostring(err))
    end
    print()
end

-- 运行 test.lua 中的测试
run_test("test.lua - test_sparse_set", test1.test_sparse_set)

-- 运行 test2.lua 中的测试
run_test("test2.lua - test_basic_operations", test2.test_basic_operations)
run_test("test2.lua - test_ecs_scenario", test2.test_ecs_scenario)

-- 打印测试总结
print("="..string.rep("=", 60))
print("Test Summary")
print("="..string.rep("=", 60))
print(string.format("Total Tests:  %d", total_tests))
print(string.format("Passed:       %d (%.1f%%)", passed_tests, (passed_tests / total_tests) * 100))
print(string.format("Failed:       %d (%.1f%%)", failed_tests, (failed_tests / total_tests) * 100))
print("="..string.rep("=", 60))

if failed_tests == 0 then
    print("🎉 All tests passed!")
    os.exit(0)
else
    print("❌ Some tests failed!")
    os.exit(1)
end
