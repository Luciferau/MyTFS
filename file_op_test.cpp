#include "file_op.h"
#include "head.h"

using namespace std;
using namespace lucifer;

int main(int argc, char* argv[])
{
    constexpr const char * filename = "file_op.txt";
    tfs::FileOperation * fileOP = new tfs::FileOperation(filename,O_LARGEFILE|O_RDWR|O_CREAT);
    int fd = fileOP->open_file();
    if(fd < 0)
    {
        cout << "open file failed" << endl;
        return -1;
    }

    char buffer[65];
    memset(buffer,'8',sizeof(buffer));

  {
      int ret = fileOP->pwrite_file(buffer,sizeof(buffer),1024);
    if(ret < 0)
    {
        cout << "pwrite file failed" << endl;
        return -1;
    }
    
  }
  memset(buffer,0,sizeof(buffer));
  {
    auto ret = fileOP->pread_file(buffer,sizeof(buffer),1024);
    if(ret < 0)
    {
        cout << "pread file failed" << endl;
        return -1;
    }
    buffer[64] = '\0';
    cout << "pread file success" << endl;
    cout << "data: " << buffer << endl;
  }
 
    fileOP->close_file();

    delete fileOP;
    return 0;
}

