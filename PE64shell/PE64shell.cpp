#include "PE.h"

int main(int argc, char* argv[]) {
    CString filePath = "D:\\c_project\\PE64shell\\coleak.exe";
    const char* pass = "coleak";
    CPE pe;
    bool result = pe.Pack(filePath, pass);
    if (result) {
        std::cout << "���ܳɹ���" << std::endl;
    }
    else {
        std::cout << "����ʧ�ܣ�" << std::endl;
    }
    return 0;
}