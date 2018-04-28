#pragma once

#include <vector>

namespace cosche
{

namespace singleton
{

struct Abstract;

class Supervisor
{

public:

    static void registerSingleton(Abstract* const singleton);

    static void clean();

private:

    static std::vector<Abstract*> _singletons;
};

} // namespace singleton

} // namespace cosche