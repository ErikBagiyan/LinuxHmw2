#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <stdexcept>

int _add(int num1, int num2)
{
    return num1 + num2;
}

int _sub(int num1, int num2)
{
    return num1 - num2;
}

int _mul(int num1, int num2)
{
    return num1 * num2;
}

int _div(int num1, int num2)
{
    if (num2 != 0)
        return num1 / num2;
    else
        throw std::runtime_error("Division by zero");
}

std::mutex mtx;
std::condition_variable cv;
int op_code = -1;
int num1 = 0;
int num2 = 0;
int result = 0;
bool done = false;

void worker()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, []{ return done; });

        try
        {
            switch (op_code)
            {
                case 0:
                    result = _add(num1, num2);
                    break;
                case 1:
                    result = _sub(num1, num2);
                    break;
                case 2:
                    result = _mul(num1, num2);
                    break;
                case 3:
                    result = _div(num1, num2);
                    break;
                default:
                    throw std::invalid_argument("Invalid operation code");
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Error: " << ex.what() << std::endl;
            exit(EXIT_FAILURE);
        }

        done = false;
        lock.unlock();
        cv.notify_one();
    }
}

int main()
{
    std::thread t(worker);
    t.detach();

    while (true)
    {
        int code = -1;
        std::cout << "Enter operation code (0=add, 1=sub, 2=mul, 3=div): ";
        std::cin >> code;

        if (code < 0 || code > 3)
        {
            std::cerr << "Invalid operation code" << std::endl;
            continue;
        }

        int n1 = 0, n2 = 0;
        std::cout << "Enter two numbers: ";
        std::cin >> n1 >> n2;

        std::unique_lock<std::mutex> lock(mtx);
        op_code = code;
        num1 = n1;
        num2 = n2;
        done = true;
        cv.notify_one();

        cv.wait(lock, []{ return !done; });

        std::cout << "Result: " << result << std::endl;
    }

    return 0;
}
