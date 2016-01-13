#ifndef _ERRORCLASS_H_
#define _ERRORCLASS_H_
#include<string>
class ErrorClass {
        private:
                int error_int;
                std::string error_txt;
        public:
		ErrorClass() {};
                ErrorClass(int a,const std::string &b) {
                        error_int = a;
                        error_txt = b;
                }
                int getErrorInt() const { return error_int; }
                std::string getErrorString() const { return error_txt; }
};
#endif
