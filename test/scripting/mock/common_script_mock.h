#ifdef HAVE_JS

#ifndef GERBERA_COMMON_SCRIPT_MOCK_H
#define GERBERA_COMMON_SCRIPT_MOCK_H
#include <duk_config.h>

// The interface used to mock the `common.js` script functions and other global functions.
// When testing scripts this mock allows for tracking of script calls and inputs.
// Each method is a global function used in the script
// Expectations can be decided by each test for the given scenario.
class CommonScriptInterface {
public:
    virtual ~CommonScriptInterface() = default;
    virtual duk_ret_t getPlaylistType(std::string type) = 0;
    virtual duk_ret_t print(std::string text) = 0;
    virtual duk_ret_t addContainerTree(std::vector<std::string> tree) = 0;
    virtual duk_ret_t createContainerChain(std::vector<std::string> chain) = 0;
    virtual duk_ret_t getLastPath(std::string path) = 0;
    virtual duk_ret_t readln(std::string line) = 0;
    virtual duk_ret_t addCdsObject(std::map<std::string, std::string> item, std::string containerChain, std::string objectType) = 0;
    virtual duk_ret_t copyObject(bool isObject) = 0;
    virtual duk_ret_t getCdsObject(std::string location) = 0;
    virtual duk_ret_t getYear(std::string year) = 0;
    virtual duk_ret_t getRootPath(std::string objScriptPath, std::string location) = 0;
    virtual duk_ret_t abcBox(std::string inputValue, int boxType, std::string divChar) = 0;
};

class CommonScriptMock : public CommonScriptInterface {
public:
    MOCK_METHOD1(getPlaylistType, duk_ret_t(std::string type));
    MOCK_METHOD1(print, duk_ret_t(std::string text));
    MOCK_METHOD1(addContainerTree, duk_ret_t(std::vector<std::string> tree));
    MOCK_METHOD1(createContainerChain, duk_ret_t(std::vector<std::string> chain));
    MOCK_METHOD1(getLastPath, duk_ret_t(std::string path));
    MOCK_METHOD1(readln, duk_ret_t(std::string line));
    MOCK_METHOD3(addCdsObject, duk_ret_t(std::map<std::string, std::string> item, std::string containerChain, std::string objectType));
    MOCK_METHOD1(copyObject, duk_ret_t(bool isObject));
    MOCK_METHOD1(getCdsObject, duk_ret_t(std::string location));
    MOCK_METHOD1(getYear, duk_ret_t(std::string year));
    MOCK_METHOD2(getRootPath, duk_ret_t(std::string objScriptPath, std::string location));
    MOCK_METHOD3(abcBox, duk_ret_t(std::string inputValue, int boxType, std::string divChar));
};
#endif //GERBERA_COMMON_SCRIPT_MOCK_H
#endif //HAVE_JS
