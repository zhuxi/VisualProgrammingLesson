//---------------------------------------------------------------------------

#include <basepch.h>

#pragma hdrstop

#include "MultiFtp.h"
#pragma package(smart_init)
//---------------------------------------------------------------------------
// ValidCtrCheck is used to assure that the components created do not have
// any pure virtual functions.
//

static inline void ValidCtrCheck(TMultiFtp *)
{
        new TMultiFtp(NULL);
}
//---------------------------------------------------------------------------
__fastcall TMultiFtp::TMultiFtp(TComponent* Owner)
        : TComponent(Owner)
{
   lock = false;
   isUseFile = false;
   runningThreadCnt = 0;
   stop = false;
   this->Owner  = Owner;
}
__fastcall TMultiFtp::~TMultiFtp()
{
    fclose(this->globalFile);
    if(this->inforImpl.fromToImpl)
      delete[] this->inforImpl.fromToImpl;
}
//---------------------------------------------------------------------------
namespace Multiftp
{
        void __fastcall PACKAGE Register()
        {
                 TComponentClass classes[1] = {__classid(TMultiFtp)};
                 RegisterComponents("System", classes, 0);
        }
}
//---------------------------------------------------------------------------
void __fastcall TMultiFtp::FreeMemory()
{
    if(this->globalFile)
       fclose(this->globalFile);
    if(this->inforImpl.fromToImpl)
      delete[] this->inforImpl.fromToImpl;
}
SOCKET __fastcall TMultiFtp::ConnectFtp(String host ,int port ,String userName ,String  pass)
{
    this->DoOnTextOut("欢迎使用funinhand多线程，断点续传软件!!");
    MultiThreadDealSocket *dealSocket = new MultiThreadDealSocket();
    SOCKET client = dealSocket->GetConnect(host,port);
    char * buffer = new char[100];
    int recLen ;
    recLen = recv(client,buffer,100,0);
    buffer[recLen]=0;
    if(client == NULL)
    {
      this->DoOnException("连接ftp服务器失败！");
      delete[] buffer;
      return NULL;
    }
    this->DoOnTextOut("连接ftp服务器成功！");
    String user = "USER  "+userName+" \r\n";
    this->DoOnTextOut(user);
    send(client,user.c_str(),user.Length(),0);
    recLen = recv(client,buffer,100,0);
    buffer[recLen]=0;
    if(GetCode(buffer) == "331")
    {
       this->DoOnTextOut("服务器要求验证密码。");
       String password = "PASS "+pass+" \r\n";
       this->DoOnTextOut(password);
       send(client,password.c_str(),password.Length(),0);
       recLen = recv(client,buffer,100,0);
        buffer[recLen]=0;
       int tryTimes = 3;
       while(GetCode(buffer) != "230" && tryTimes > 0)
       {
          send(client,password.c_str(),password.Length(),0);
          recLen = recv(client,buffer,100,0);
           buffer[recLen]=0;
          tryTimes --;
          this->DoOnTextOut("第"+IntToStr(3-tryTimes)+"尝试");
       }
       if(tryTimes < 0)
       {
           this->DoOnException(userName +"登录失败！");
           delete[] buffer;
           return NULL;
       }
       else
          this->DoOnTextOut("登录成功！");
    }
    char *type = "TYPE I \r\n";
    this->DoOnTextOut(type);
    send(client,type,strlen(type),0);
    recLen = recv(client,buffer,100,0);
     buffer[recLen]=0;
    this->DoOnTextOut(buffer);
    delete[] buffer;
    return client;
}
void __fastcall TMultiFtp::SetStop(bool stop)
{
  this->stop  = stop;
}
void __fastcall TMultiFtp::DoOnComplete()
{
  if(this->FOnComplete)
     this->FOnComplete(this->Owner);
}
void __fastcall TMultiFtp::DoOnGetFileSize(DWORD fileSize)
{
   if(this->FOnGetFileSize)
      this->FOnGetFileSize(this->Owner,fileSize);
}
void __fastcall TMultiFtp::DoOnTextOut(String text)
{
 if(this->FOnTextOut)
  {
     int index = text.Pos("\r\n");
     if(index > 0)
       this->FOnTextOut(this->Owner,text.SubString(1,index-1));
     else
       this->FOnTextOut(this->Owner,text);
  }
}
void __fastcall TMultiFtp::DoOnException(String error)
{
   if(this->FOnException)
   {
      int index = error.Pos("\r\n");
      if(index > 0)
      {
        this->FOnException(this->Owner,error.SubString(1,index-1));
      }
      else
      {
          this->FOnException(this->Owner,error);
      }
   }
}
void __fastcall TMultiFtp::DoOnProgress(DWORD pos)
{
  if(this->FOnProgress)
    this->FOnProgress(this->Owner,pos);
}
void  __fastcall TMultiFtp::DoOnGetRate(DWORD cnt)
{
   if(this->FOnGetRate)
      this->FOnGetRate(this->Owner,cnt);
}
String __fastcall TMultiFtp::GetCode(String revStr)
{
  String str;
  int index ;
  index = revStr.Pos(" ");
  str = revStr.SubString(1,index-1);
  return str;
}
DWORD __fastcall TMultiFtp::GetFileSize()
{
   return this->fileSize;
}
DWORD __fastcall TMultiFtp::GetFtpFileSize(SOCKET client ,String fileName)
{
   String  size = "SIZE "+this->FFileName+" \r\n";
   this->DoOnTextOut(size);
   send(client,size.c_str(),size.Length(),0);
   char *buffer= new char[100];
   int recLen ;
   recLen = recv(client,buffer,100,0);
   buffer[recLen]=0;
   this->DoOnTextOut(buffer);
   if(GetCode(buffer) != "213")
   {
     this->DoOnTextOut("尝试第二次获取");
     send(client,size.c_str(),size.Length(),0);
     recLen = recv(client,buffer,100,0);
      buffer[recLen]=0;
      this->DoOnTextOut(buffer);
     if(GetCode(buffer) != "213")
     {
        this->DoOnTextOut("尝试第三次获取");
        send(client,size.c_str(),size.Length(),0);
        recLen = recv(client,buffer,100,0);
         buffer[recLen]=0;
         this->DoOnTextOut(buffer);
        if(GetCode(buffer) != "213")
        {
            this->DoOnException("获取文件大小失败！");
            delete[] buffer;
             return 0;
        }
        else
        {
            this->DoOnTextOut(buffer);
        }
     }
     else
     {
         this->DoOnTextOut(buffer);
     }
   }
   else
   {
       this->DoOnTextOut(buffer);
   }
   String recvStr(buffer);
   delete[] buffer;
   int index =  recvStr.Pos("\r\n");
   recvStr = recvStr.SubString(1,index-1);
   index = recvStr.Pos(" ");
   recvStr = recvStr.SubString(index+1,recvStr.Length()- index);
   this->fileSize  =StrToInt(recvStr);
   return this->fileSize;
}
bool __fastcall  TMultiFtp::CheckIsSupportMulti(SOCKET client)
{
   String rest = "REST 100 \r\n";
   char *buffer = new char[100];
   this->DoOnTextOut(rest);
   int recLen ;
   send(client,rest.c_str(),rest.Length(),0);
   recLen = recv(client,buffer,100,0);
   buffer[recLen]=0;
   this->DoOnTextOut(buffer);
   if(GetCode(buffer) != "350")
   {
       delete[] buffer;
       return false;
   }
   delete[] buffer;
   return true;
}
FileTail * __fastcall TMultiFtp::GetFilePosLen()
{
   while(lock)
   {
     Sleep(50);
   }
   lock = true;
   FileTail * temp  = new FileTail[1];
   if(this->currentFilePos >= fileSize)
   {
       lock = false;
       temp->isOk = true;
       return temp;
   }
   if(this->FPerGetLen + this->currentFilePos > fileSize)
   {
       temp->pos = ConvertIntToStr(this->currentFilePos);
       temp->len = ConvertIntToStr(this->fileSize - this->currentFilePos);
       temp->isOk = false;
       this->currentFilePos = fileSize;
       lock = false;
       return temp;
   }
   temp->pos = ConvertIntToStr(this->currentFilePos);
   temp->len = ConvertIntToStr(this->FPerGetLen);
   temp->isOk = false;
   this->currentFilePos = this->currentFilePos + this->FPerGetLen;
   lock = false;
   return  temp;
}
String __fastcall TMultiFtp::ConvertIntToStr(DWORD len)
{
     String str;
     str = IntToStr(len);
     int strLen;
     strLen = str.Length();
     for(int i =0 ; i < 10 -strLen ; i++)
     {
        str = "0"+str;
     }
     return str;
}
bool __fastcall TMultiFtp::ChangeDirectory(SOCKET client ,String dirName)
{
    String dir =  "CWD "+dirName+" \r\n";
    this->DoOnTextOut(dir);
    char *buffer = new char[100];
    int recLen ;
    send(client,dir.c_str(),dir.Length(),0);
    recLen = recv(client,buffer,100,0);
    buffer[recLen]=0;
    if(GetCode(buffer) != "250")
    {
        delete[] buffer;
        this->DoOnException(buffer);
        return false;
    }
    this->DoOnTextOut(buffer);
    delete[] buffer;
    return true;
}
void __fastcall TMultiFtp::StartDownloadFile()
{
   SOCKET client = this->ConnectFtp(this->FHost,this->FPort,this->FUserName,this->FPass);
   this->DoOnTextOut("下载的线程模块数为："+IntToStr(this->FThreadCnt));
   if(!client)
   {
       return;
   }  //连接ftp成功
   if(!this->CheckIsSupportMulti(client))
   {
      this->DoOnTextOut("该站点不支持断点续传。");
      this->FThreadCnt =  1;     //如果ftp服务器不支持多线程下载
   }
   this->DoOnTextOut("该站点支持断点续传。");
   ////////////将当前的工作目录设置成
   this->FileName = this->SetCurrentDir(client,this->FilePath);
   this->GetFtpFileSize(client,this->FileName);
   this->DoOnGetFileSize(this->fileSize);  //输出文件大小
   if(this->fileSize == 0)
   {
      closesocket(client);
      return;
   }
   if(this->fileSize <= (DWORD)this->PerGetLen)
   {
      this->FThreadCnt = 1;
   }
   if(this->FileName == "")
   {
      closesocket(client);
      return;
   }
   String hisFileName = this->FileName + ".san";
   ///////////////////检测是否断点续传/////////////////////////
   if(FileExists(hisFileName)&& (GetFileSizeByName(hisFileName) > 0)) //如果文件存在
   {
      this->GetInfor(hisFileName);
   }
   else
   {
     globalFile = fopen(hisFileName.c_str(),"w+b");
     if(globalFile == NULL)
     {
      this->DoOnException("创建文件"+hisFileName+"失败");
      closesocket(client);
          return ;
     }
     this->FilePos = this->perFilePos =  0;
     this->CreateNewFile(this->LocalLoad,this->fileSize);
     DivisionFile();  ////
   }
///////////////////////////////////////////////////////////
   if(this->inforImpl.alreadyDownloadCnt >= this->fileSize)
   {
       fclose(globalFile);
       DeleteFile(hisFileName);
       closesocket(client);
       return;
   }
   //////////////把文件分割成几块进行下载 ///////////
    this->DoOnTextOut("下载的线程模块数为："+IntToStr(this->FThreadCnt));
   ////////////////////////////启动线程0//////////////
     MultiFtpDownloadThread *thread = new MultiFtpDownloadThread(true);
     this->runningThreadCnt ++;
     if(this->stop)
     {
        closesocket(client);
        return;
     }
     thread->parent = this;
     thread->commandClient = client;
     thread->localFileLoad = this->FLocalLoad;
     thread->FOnComplete = this->FOnComplete;
     thread->FOnException  = this->FOnException;
     thread->FOnProgress = this->FOnProgress;
     thread->FOnTextOut = this->FOnTextOut;
     thread->FileName = this->FileName;
     thread->Owner = this->Owner;
     thread->perFileLen = this->PerGetLen;
     thread->ID =  0;
     thread->Resume();
   //////////////////////////////////////////////////////
   for(int i = 1; i < FThreadCnt ; i++)
   {
       SOCKET temp = this->ConnectFtp(this->FHost,this->FPort,this->FUserName,this->FPass);
       this->CreateThread(i,temp);
   }
   ////////////保证有多个线程同步下载/////////////
   while(true && !this->stop)
   {
        if(this->runningThreadCnt < this->FThreadCnt)
        {
            if(this->FilePos >= this->fileSize)
              return;
            int successCode = this->GetSuccessCode();  //取得已经完成的线程id
            int busyCode = this->GetBusyCode();  //取得任务最重的线程id
            if((this->inforImpl.fromToImpl[busyCode].to - this->inforImpl.fromToImpl[busyCode].from)< (DWORD)this->PerGetLen )
            { //如果当前的文件下载字节比一次最小要取得的字节数还小，就退出
                return;
            }
            AverageDownload(successCode,busyCode);
            Sleep(1000);
        }
        else
        {
           Sleep(1000);
        }
   }

}
DWORD __fastcall TMultiFtp::GetFileSizeByName(String fileName)
{
    FILE *file ;
    DWORD  dataLen;
    file = fopen(fileName.c_str(),"r+b");
    if(file ==  NULL) return 0;
    fseek(file,0,2);
    dataLen = ftell(file);
    fclose(file);
    return dataLen;
}
void __fastcall TMultiFtp::AverageDownload(int successCode , int busyCode)
{
   DWORD per  = (this->inforImpl.fromToImpl[busyCode].to - this->inforImpl.fromToImpl[busyCode].from - this->FPerGetLen) /2;
   DWORD successFrom = this->inforImpl.fromToImpl[busyCode].to - per;
   this->inforImpl.fromToImpl[successCode].from = successFrom;
   this->inforImpl.fromToImpl[successCode].to = this->inforImpl.fromToImpl[busyCode].to;
   this->inforImpl.fromToImpl[busyCode].to = successFrom;
   SOCKET temp = this->ConnectFtp(this->FHost,this->FPort,this->FUserName,this->FPass);   //开始下载
   this->CreateThread(successCode,temp);
}
void  __fastcall   TMultiFtp::CreateThread(int code ,SOCKET client)
{
       MultiFtpDownloadThread *thread = new MultiFtpDownloadThread(true);
       this->runningThreadCnt ++;
       this->SetCurrentDir(client,this->FilePath);
       if(this->stop)
       {
         closesocket(client);
          return;
       }
       thread->parent = this;
       thread->commandClient = client;
       thread->localFileLoad = this->FLocalLoad;
       thread->FOnComplete = this->FOnComplete;
       thread->FOnException  = this->FOnException;
       thread->FOnProgress = this->FOnProgress;
       thread->FOnTextOut = this->FOnTextOut;
       thread->FileName = this->FileName;
       thread->Owner = this->Owner;
       thread->perFileLen = this->PerGetLen;
       thread->ID =  code;
       thread->Resume();
}
String __fastcall  TMultiFtp::SetCurrentDir(SOCKET client ,String fileName)
{
    int index;
    index = fileName.Pos("/");
    String temp;
    char *buffer = new char[100];
    while(index > 0)
    {
      temp = fileName.SubString(1,index-1);
      String curDir = "PWD \r\n";
      send(client,curDir.c_str(),curDir.Length(),0);
      int recLen = recv(client,buffer,100,0);
      buffer[recLen] = 0;
      if(!this->ChangeDirectory(client,temp))
      {
          delete[] buffer;
          return "";
      }
      fileName = fileName.SubString(index+1,fileName.Length()- index);
      index = fileName.Pos("/");
    }
    delete[] buffer;
    return fileName;
}
void __fastcall TMultiFtp::WriteToFile(String filePath,DWORD pos ,char *buffer , int len)
{
   String tempFileName = filePath + ".nam";
    while(isUseFile)
       Sleep(50);
    isUseFile = true;
    FILE *file ;
    file = fopen(tempFileName.c_str(),"r+b");
    if(file == NULL) {
    String str  = this->FileName + ".san";
    fclose(this->globalFile);
    DeleteFile(str);
    this->DoOnException("写入文件失败！");
    isUseFile = false;
    return;
    } ;
    fseek(file,pos,0);
    fwrite(buffer,sizeof(char),len,file);
    fclose(file);
    isUseFile = false;
}
bool __fastcall TMultiFtp::CreateNewFile(String fileName, DWORD size)
{
   String tempFileName = fileName + ".nam";
   FILE  *file;
   file = fopen(tempFileName.c_str(),"w+b");
   if(file == NULL) return false;
   this->DoOnTextOut("正在创建文件,请稍候...");
   char * buffer  = new char[60000];
   memset(buffer,'0',60000);
   DWORD writeLen = 0;
   int needLen = 60000;
   while(writeLen < size)
   {
     if(writeLen + 60000 > size)
     {
        needLen = size -writeLen;
     }
     else
        needLen = 60000;
     int len =  fwrite(buffer,sizeof(char),needLen,file);
     if(len > 0)  writeLen += len;
  }
  delete[]  buffer;
    fclose(file);
    return true;
}
bool __fastcall TMultiFtp::WriteInforToFile()
{
   String writeStr ;
   String fileSizeStr = IntToStr(this->fileSize);
   String threadCntStr = IntToStr(this->inforImpl.threadCnt);
   String downloadCntStr = IntToStr(this->FilePos);
   writeStr = this->LocalLoad+"\r\n"+fileSizeStr+"\r\n"+threadCntStr+"\r\n"+downloadCntStr+"\r\n";
   for(int i = 0; i< this->inforImpl.threadCnt ; i++)
   {
     writeStr += IntToStr(this->inforImpl.fromToImpl[i].from) +"-"+IntToStr(this->inforImpl.fromToImpl[i].to)+"\r\n";
   }
   while(this->FileLocked) Sleep(10);
   FileLocked = true;
   if(this->globalFile == NULL)
   {
     FileLocked = false;
     return false;
   }
   try
   {
     fseek(this->globalFile,0,0);
     fwrite(writeStr.c_str(),sizeof(char),writeStr.Length(),this->globalFile);
     FileLocked = false;
     return true;
   }
   catch(...)
   {
       FileLocked = false;
     return false;
   }

}
void __fastcall  TMultiFtp::DivisionFile()
{
  this->fromToImpl = new FromToImpl[this->FThreadCnt];
  this->inforImpl.fileLoad = this->FLocalLoad;
  this->inforImpl.fileSize = this->fileSize;
  this->inforImpl.threadCnt = this->FThreadCnt;
  this->inforImpl.alreadyDownloadCnt = this->FilePos;
   this->inforImpl.fromToImpl = new FromToImpl[this->FThreadCnt];
  DWORD perCnt = this->fileSize / this->FThreadCnt;
  int i;
  for(i = 0 ;i < this->FThreadCnt-1 ; i++)
  {
     this->inforImpl.fromToImpl[i].from =  perCnt * i;
     this->inforImpl.fromToImpl[i].to = perCnt*(i+1);
  }
     this->inforImpl.fromToImpl[i].from =  perCnt*(i);
     this->inforImpl.fromToImpl[i].to = this->fileSize;
    this->WriteInforToFile();
}
void __fastcall TMultiFtp::DealTimer(MSG msg)
{
   DWORD desLen = this->FilePos  - this->perFilePos;
   this->DoOnGetRate(desLen); 
}
void __fastcall  TMultiFtp::GetInfor(String fileName)
{
   char *buffer = new char[5000];
   while(this->FileLocked) Sleep(50);
   this->FileLocked = true;
   globalFile = fopen(fileName.c_str(),"r+b");
   fseek(this->globalFile,0,2);
   int fileLen = ftell(this->globalFile);
   fseek(this->globalFile,0,0);
   int readLen = fread(buffer,sizeof(char),fileLen,this->globalFile);
   buffer[readLen] = 0;
   String str(buffer);
   this->hisFileStr = str;
   delete[] buffer;
  this->FileLocked = false;
  CreateInforImpl(str);
}
void __fastcall TMultiFtp::CreateInforImplFromString(String inforStr)
{
   int index;
   String temp;
   index = hisFileStr.Pos("\r\n");
   this->FLocalLoad =  hisFileStr.SubString(1,index-1);             //获取了文件的保存地址
   hisFileStr = hisFileStr.SubString(index+2,hisFileStr.Length()-index);
   index = hisFileStr.Pos("\r\n");
   this->fileSize = StrToInt(hisFileStr.SubString(1,index-1)); //获取了文件大小
   hisFileStr = hisFileStr.SubString(index+2,hisFileStr.Length() -index);
   index = hisFileStr.Pos("\r\n");
   this->ThreadCnt = StrToInt(hisFileStr.SubString(1,index-1)); //获取了线程数目
   hisFileStr = hisFileStr.SubString(index+2,hisFileStr.Length() - index);
   index = hisFileStr.Pos("\r\n");
   this->FilePos  = StrToInt(hisFileStr.SubString(1,index-1));             //获取了已经下载文件的大小
   hisFileStr = hisFileStr.SubString(index+2,hisFileStr.Length() - index);
   ////付值给全局变量
   this->inforImpl.fromToImpl = new FromToImpl[ThreadCnt];
   this->inforImpl.fileLoad = this->FLocalLoad;
   this->inforImpl.fileSize = this->fileSize;
   this->inforImpl.threadCnt = this->ThreadCnt;
   this->inforImpl.alreadyDownloadCnt = this->FilePos;
   String from ,to;
   for(int i =0 ;i < this->ThreadCnt ; i++)
   {
      index = hisFileStr.Pos("\r\n");
      temp = hisFileStr.SubString(1,index-1); // 获取from  - to 值
      hisFileStr = hisFileStr.SubString(index +2 ,hisFileStr.Length() - index);
      index = temp.Pos("-");
      from  = temp.SubString(1,index-1);
      to = temp.SubString(index+1,temp.Length() - index);
      this->inforImpl.fromToImpl[i].from = StrToInt(from);
      this->inforImpl.fromToImpl[i].to = StrToInt(to);
   }
}
void __fastcall TMultiFtp::CreateInforImpl(String str)
{     //一些断点续传的参数
   int index;
   String temp;
   index = hisFileStr.Pos("\r\n");
   this->FLocalLoad =  hisFileStr.SubString(1,index-1);             //获取了文件的保存地址
   hisFileStr = hisFileStr.SubString(index+2,hisFileStr.Length()-index);
   index = hisFileStr.Pos("\r\n");
   this->fileSize = StrToInt(hisFileStr.SubString(1,index-1)); //获取了文件大小
   hisFileStr = hisFileStr.SubString(index+2,hisFileStr.Length() -index);
   index = hisFileStr.Pos("\r\n");
   this->ThreadCnt = StrToInt(hisFileStr.SubString(1,index-1)); //获取了线程数目
   hisFileStr = hisFileStr.SubString(index+2,hisFileStr.Length() - index);
   index = hisFileStr.Pos("\r\n");
   this->FilePos  = StrToInt(hisFileStr.SubString(1,index-1));             //获取了已经下载文件的大小
   hisFileStr = hisFileStr.SubString(index+2,hisFileStr.Length() - index);
   ////付值给全局变量
   this->inforImpl.fromToImpl = new FromToImpl[ThreadCnt];
   this->inforImpl.fileLoad = this->FLocalLoad;
   this->inforImpl.fileSize = this->fileSize;
   this->inforImpl.threadCnt = this->ThreadCnt;
   this->inforImpl.alreadyDownloadCnt = this->FilePos;
   String from ,to;
   for(int i =0 ;i < this->ThreadCnt ; i++)
   {
      index = hisFileStr.Pos("\r\n");
      temp = hisFileStr.SubString(1,index-1); // 获取from  - to 值
      hisFileStr = hisFileStr.SubString(index +2 ,hisFileStr.Length() - index);
      index = temp.Pos("-");
      from  = temp.SubString(1,index-1);
      to = temp.SubString(index+1,temp.Length() - index);
      this->inforImpl.fromToImpl[i].from = StrToInt(from);
      this->inforImpl.fromToImpl[i].to = StrToInt(to);
   }
}
int __fastcall TMultiFtp::GetSuccessCode()   //返回已经完成的线程id
{
    for(int i = 0 ;i < this->FThreadCnt; i ++)
    {
       if(this->inforImpl.fromToImpl[i].from == this->inforImpl.fromToImpl[i].to)
       {
         return i;
       }
    }
    return -1;
}
int __fastcall TMultiFtp::GetBusyCode() //返回任务最多的线程id
{
   int code = -1 ;
   DWORD descLen =0 ;
    for(int i = 0; i < this->FThreadCnt; i++)
    {
         if( (this->inforImpl.fromToImpl[i].to - this->inforImpl.fromToImpl[i].from ) > descLen)
           {
              descLen =  this->inforImpl.fromToImpl[i].to - this->inforImpl.fromToImpl[i].from ;
              code= i;
           }
    }
    return code;
}
