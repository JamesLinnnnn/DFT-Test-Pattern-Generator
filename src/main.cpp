#include "readckt.cpp"
#include <string>
#include <sstream>
#include <algorithm>

int main()
{
   int com;
   string cline,wstr, temp;


   while(!Done) {
      printf("\nCommand>");
      std::getline(std::cin, cline);
      istringstream iss(cline);
      
      if(!(iss >> wstr)) continue;

      // �ഫ wstr ���j�g
      
      std::transform(wstr.begin(), wstr.end(), wstr.begin(), ::toupper);
      
      // ����R�O�᪺�Ѿl����

      if (std::getline(iss >> std::ws, temp)) 
      {
         temp = temp.substr(temp.find_first_not_of(" \t"));
         cp = temp;
      } 
      else 
      {
         cp = "";  // �S���B�~�Ѽ�
      }
      // std::cout<<cp<<std::endl;
      com = READ;
      while(com < command.size() && wstr!=command[com].name) com++;
      // std::cout<<"com = "<<com<<std::endl;
      if(com < command.size()) {
         if(command[com].state <= Gstate) 
         {
            // std::cout<<"get in"<<std::endl;
            (*command[com].fptr)();
         }
         else 
         {
            printf("Execution out of sequence!\n");
         }
      }
      else 
      {
         system(cline.c_str());
      }
   }
}
