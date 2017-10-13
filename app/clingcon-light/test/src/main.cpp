#include "clingo.hh"
#include "test/testapp.h"



int main(int argc, char *argv[])
{
    int ret;
    {
        if (argc==1)
        {
            std::cout << clingcon::Adder::singleton().size() << " Tests" << std::endl;
            return clingcon::Adder::singleton().size();
        }
        clingcon::TestApp app(strtol(argv[1],nullptr,10));
        char* empty[] = {"--quiet=2,2,2", "--verbose=0", "0","--outf=3"}; /// probably bad idea ?
        ret = Clingo::clingo_main(app, {empty, sizeof(empty)/sizeof(char*)});
        if (!app.retValue())
            return 1;
    }

    return 0;
}
