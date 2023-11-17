#include "graphic/VWindow.h"

#include <iostream>
int main() {
    V::Window app;
    try
    {
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cout << "ERROR: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}
