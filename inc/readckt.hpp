#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <cmath>
#include <queue>
#include <stack>
#include <map>
#include <set>
#include <chrono>
using namespace std;

#define Upcase(x) ((isalpha(x) && islower(x))? toupper(x) : (x))
#define Lowcase(x) ((isalpha(x) && isupper(x))? tolower(x) : (x))

enum e_com {READ, PC, HELP, QUIT,LEV,LOGICSIM,RTPG,RFL,PFS,DFS,SCOAP,DALG,PODEM,TPG};
enum e_state {EXEC, CKTLD};         /* Gstate values */
enum e_ntype {GATE, PI, FB, PO};    /* column 1 of circuit format */
enum e_gtype {IPT, BRCH, XOR, OR, NOR, NOT, NAND, AND,XNOR,BUF};  /* gate types */

class cmdclass
{
    public:
        string name;        /* command syntax */
        void (*fptr)();            /* function pointer of the commands */
        enum e_state state;        /* execution state sequence */
        cmdclass(string name , void (*f)(), e_state t) : name(name), fptr(f), state(t) {}       //initialize the var
};

class node_class
{
        
    public:
        unsigned indx;             /* node index(from 0 to NumOfLine - 1 */
        unsigned num;              /* line number(May be different from indx */
        enum e_ntype ntype;
        enum e_gtype type;         /* gate type */
        unsigned fin;              /* number of fanins */
        unsigned fout;             /* number of fanouts */
        vector<node_class *>unodes;   /* pointer to array of up nodes */
        vector<node_class *>dnodes;   /* pointer to array of down nodes */
        vector<int> dfs_detect;
        int level;                 /* level of the gate output */
        int D=-1;
        int value=-1;               /*0->0, 1->1, 2->x*/
        int CC0;
        int CC1;
        int CO;
        void Simulate();
        void backward_Simulate();
        int Unodes_x_count();
        int Unodes_1_count();
        int Unodes_0_count();
        bool Dnodes_has_BRCH();
        vector<int> Unodes_D_Dbar_count();  
        bool forward_check();  
        bool backward_check();
        int CC0_CC1_calculation();
        void CO_caculation();

};

bool node_class::Dnodes_has_BRCH()
{
    for (unsigned i = 0; i < dnodes.size(); i++) 
    {
        if (dnodes[i]->type == BRCH) 
        {
            return true;
        }
    }
    return false;    
}
void node_class::Simulate()
{
    int x_count = Unodes_x_count();
    int count1 = Unodes_1_count();
    bool x = (bool)x_count;
    int temp;
    vector<int> D_Dbar_count = Unodes_D_Dbar_count();
    int D_Dbar_count_tot = D_Dbar_count[0] + D_Dbar_count[1];
    int D_odd = ((D_Dbar_count[0] - D_Dbar_count[1])%2+2)%2;  
    bool D_more = (D_Dbar_count[0] > D_Dbar_count[1]);
    int offset = min(D_Dbar_count[0], D_Dbar_count[1]);
    int odd1 = (count1 + offset)%2;   
    if(x_count==unodes.size())                  //all unodes is x  so the output is x too
    {
        value = 2;
        D = -1;
        return;
    }
    if(D>=0)return;                             //if Im already a fault, I don't need to simulate let the check func to check correct or not
    switch(type)
    {
        case BRCH:
            if(D>=0)
            {
                value=value;
                D = D;
            }
            else
            {
                value = (unodes[0]->value);
                D = (unodes[0]->D);
            }
            break;
        case XOR:
            for(int i=0;i<unodes.size();i++)    //if u nodes contain x then output is x
            {
                if(unodes[i]->value==2&&D==-1)
                {
                    value = 2;
                    D = -1;
                    return;
                }
            }

            if ((D_Dbar_count_tot == 0) || (D_odd == 0)){
                if (odd1 == 0){
                    value = 0;
                    break;
                }
                else {
                    value = 1;
                    break;
                }
            }
            else{
                if (odd1 == 0){
                    if (D_more){
                        D = 1;
                        value = 0;
                        break;
                    }
                    else{
                        D = 0;
                        value = 1;
                        break;
                    }
                }
                else {
                    if (D_more){
                        D = 0;
                        value = 1;
                        break;
                    }
                    else{
                        D = 1;
                        value = 0;
                        break;
                    }
                }
            }
            break;
        case OR:
            temp = 0;
            if(D_Dbar_count[0]>0 && D_Dbar_count[1]>0)   //if unodes both contain D, D' than it must be 1 and won't propogate fault
            {
                value = 1;
                D = D;                    
                return;
            }
            for(int i=0;i<unodes.size();i++)
            {
                if(unodes[i]->value!=2&&unodes[i]->D==-1)
                {
                    temp |=unodes[i]->value;
                }
                if(temp==1)                            //if unodes has 1 than value=1, won't propogate the fault
                {
                    value = 1;
                    D = -1;
                    return;
                }
            }
            if(x==0)                                    //if unodes has no x and value is {0,D}=>D, {0,D'}=>D'
            {
                if(D_Dbar_count[0]>0) 
                {
                    D = 1;
                    value = 0;
                }
                else if (D_Dbar_count[1]>0)
                {
                    D=0;
                    value = 1;
                }
                else value = temp;
            } 
            else if(temp==0 && x==1)                    //if Unodes has x and all value is 0 (for safety) output is x, won't propogate the fault
            {
                value = 2;
                D = D;
            }
            break;

        case NOR:
            temp = 0;
            if(D_Dbar_count[0]>0 && D_Dbar_count[1]>0)   //if unodes both contain D, D' than it must be 0 and won't propogate fault
            {
                value = 0;
                D = D;                    
                return;
            }
            for(int i=0;i<unodes.size();i++)
            {
                if(unodes[i]->value!=2&&unodes[i]->D==-1)
                {
                    temp |=unodes[i]->value;
                }
                if(temp==1)                            //if unodes has 1 than value=0, won't propogate the fault
                {
                    value = !temp;
                    D = -1;
                    return;
                }
            }
            if(x==0)                                    //if unodes has no x and value is {0,D}=>D', {0,D'}=>D
            {
                value = !temp;
                if(D_Dbar_count[0]>0) 
                {
                    D = 0;
                    value = 1;
                }
                else if (D_Dbar_count[1]>0)
                {
                    D = 1;
                    value = 0;
                }
                else  value = !temp;
            } 
            else if(temp==0 && x==1)                    //if Unodes has x and value is 0 (for safety) output is x, won't propogate the fault
            {
                value = 2;
                D = D;
            }
            break;

        case NOT:
            if(unodes[0]->D>=0)
            {
                value = !(unodes[0]->value);
                D = !(unodes[0]->D);
            }
            else                                    //if unodes is not D but urself is fault, then still be the same
            {
                value = !unodes[0]->value;
                D = -1;
            }
            break;

        case NAND:
            temp = 1;
            if(D_Dbar_count[0]>0 && D_Dbar_count[1]>0)   //if unodes both contain D, D' than it must be 0 and won't propogate fault
            {
                value = 1;
                D = D;                    
                return;
            }
            for(int i=0;i<unodes.size();i++)
            {
                if(unodes[i]->value!=2&&unodes[i]->D==-1)
                {
                    temp &=unodes[i]->value;
                }
                if(temp==0)                            //if unodes has 0 than value=0, won't propogate the fault
                {
                    
                    value = !temp;
                    D = -1;
                    return;
                }
            }
            if(x==0)                                    //if unodes has no x and value is {1,D}=>D, {1,D'}=>D'
            {
                value = !temp;
                if(D_Dbar_count[0]>0) 
                {
                    D = 0;
                    value = 1;
                }
                else if (D_Dbar_count[1]>0)
                {
                    D = 1;
                    value = 0;
                }
                else value = !temp;
            } 
            else if(temp==1 && x==1)                    //if Unodes has x and value is 1 (for safety) output is x, won't propogate the fault
            {
                value = 2;
                D=-1;
            }
            break;

        case AND:
            temp = 1;
            // cout<<"num:"<<num<<endl;
            // cout<<"D:"<<D_Dbar_count[0]<<endl;
            // cout<<"D_bar: "<<D_Dbar_count[1]<<endl;
            if(D_Dbar_count[0]>0 && D_Dbar_count[1]>0)   //if unodes both contain D, D' than it must be 0 and won't propogate fault
            {
                value = 0;
                D = D;                    
                return;
            }
            for(int i=0;i<unodes.size();i++)
            {
                if(unodes[i]->value!=2&&unodes[i]->D==-1)
                {
                    temp &=unodes[i]->value;
                }
                if(temp==0)                            //if unodes has 0 than value=0, won't propogate the fault
                {
                    value = temp;
                    D = -1;
                    return;
                }
            }
            if(x==0)                                    //if unodes has no x and value is {1,D}=>D, {1,D'}=>D'
            {
                value = temp;
                if(D_Dbar_count[0]>0) 
                {
                    D = 1;
                    value = 0;
                }
                else if (D_Dbar_count[1]>0)
                {
                    D=0;
                    value = 1;
                }
                else  value = temp;
            } 
            else if(temp==1 && x==1)                    //if Unodes has x and value is 1 (for safety) output is x, won't propogate the fault
            {
                value = 2;
                D=-1;
            }
            break;

        case XNOR:
            for(int i=0;i<unodes.size();i++)    //if u nodes contain x then output is x
            {
                if(unodes[i]->value==2&&D==-1)
                {
                    value = 2;
                    D = -1;
                    return;
                }
            }

            if ((D_Dbar_count_tot == 0) || (D_odd == 0)){
                if (odd1 == 0){
                    value = 1;
                    break;
                }
                else {
                    value = 0;
                    break;
                }
            }
            else{
                if (odd1 == 0){
                    if (D_more){
                        D = 0;
                        value = 1;
                        break;
                    }
                    else{
                        D = 1;
                        value = 0;
                        break;
                    }
                }
                else {
                    if (D_more){
                        D = 1;
                        value = 0;
                        break;
                    }
                    else{
                        D = 0;
                        value = 1;
                        break;
                    }
                }
            }
        break;
        case BUF:
            value = (unodes[0]->value);
            D = (unodes[0]->D);
            break;
    }
    return;
}
void node_class::backward_Simulate()
{
    bool I_am_fault = (D>=0);
    int x_count = Unodes_x_count();
    int count1 = Unodes_1_count();
    int count0 = Unodes_0_count();     
    vector<int> D_Dbar_count = Unodes_D_Dbar_count();
    int D_Dbar_count_tot = D_Dbar_count[0] + D_Dbar_count[1];
    int D_odd = ((D_Dbar_count[0] - D_Dbar_count[1])%2+2)%2;  
    bool D_more = (D_Dbar_count[0] > D_Dbar_count[1]);
    int offset = min(D_Dbar_count[0], D_Dbar_count[1]);
    int odd1 = (count1 + offset)%2;   
    switch(type)
    {
        case IPT:
            break;
        case BRCH:
            if(unodes[0]->D>=0)  unodes[0]->value = value;
            else unodes[0]->value = value^I_am_fault;
            break;
        case NOT:
            if(unodes[0]->D>=0)  unodes[0]->value = !value;
            else unodes[0]->value = !(value^I_am_fault);
            break;
        case BUF:
            if(unodes[0]->D>=0)  unodes[0]->value = value;
            else unodes[0]->value = value^I_am_fault;
            break;
        case XOR:
        case XNOR:
                if (x_count == 1){
                        if (!I_am_fault){ //output node 1/0
                            if ((value == 0 && type == XOR) || (value == 1 && type == XNOR)){
                                if (D_Dbar_count_tot == 0 || D_odd == 0){
                                    if (odd1 == 0){
                                        for(int i=0; i<unodes.size(); i++){
                                            if(unodes[i]->value==2) {
                                                unodes[i]->value=0;
                                                break;
                                            }
                                        }   
                                    }
                                    else{ //odd1 != 0
                                        for(int i=0; i<unodes.size(); i++){
                                            if(unodes[i]->value==2) {
                                                unodes[i]->value=1;
                                                break;
                                            }
                                        }   
                                    }
                                }
                                else if (!D_Dbar_count_tot == 0 && D_odd == 1){
                                    if (odd1 == 0){
                                        for(int i=0; i<unodes.size(); i++){
                                            if(unodes[i]->value==2){
                                                if (D_more){
                                                    // unodes[i]->D=1;
                                                    unodes[i]->value=1;
                                                    break;
                                                }
                                                else {
                                                    // unodes[i]->D=0;
                                                    unodes[i]->value=0;
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                    else{ //odd1 != 0
                                        for(int i=0; i<unodes.size(); i++){
                                            if(unodes[i]->value==2){
                                                if (D_more){
                                                    // unodes[i]->D=0;
                                                    unodes[i]->value=0;
                                                    break;
                                                }
                                                else {
                                                    // unodes[i]->D=1;
                                                    unodes[i]->value=1;
                                                    break;
                                                }
                                            }
                                        }
                                    }  
                                }
                            }
                            else if ((value == 1 && type == XOR) || (value == 0 && type == XNOR)){
                                if (D_Dbar_count_tot == 0 || D_odd == 0){
                                    if (odd1 == 0){
                                        for(int i=0; i<unodes.size(); i++){
                                            if(unodes[i]->value==2) {
                                                unodes[i]->value=1;
                                                break;
                                            }
                                        }   
                                    }
                                    else{ //odd1 != 0
                                        for(int i=0; i<unodes.size(); i++){
                                            if(unodes[i]->value==2) {
                                                unodes[i]->value=0;
                                                break;
                                            }
                                        }   
                                    }
                                }
                                else if (!D_Dbar_count_tot == 0 && D_odd == 1){
                                    if (odd1 == 0){
                                        for(int i=0; i<unodes.size(); i++){
                                            if(unodes[i]->value==2){
                                                if (D_more){
                                                    // unodes[i]->D=0;
                                                    unodes[i]->value=0;
                                                    break;
                                                }
                                                else {
                                                    // unodes[i]->D=1;
                                                    unodes[i]->value=1;
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                    else{ //odd1 != 0
                                        for(int i=0; i<unodes.size(); i++){
                                            if(unodes[i]->value==2){
                                                if (D_more){
                                                    // unodes[i]->D=1;
                                                    unodes[i]->value=1;
                                                    break;
                                                }
                                                else {
                                                    // unodes[i]->D=0;
                                                    unodes[i]->value=0;
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else{ //output node D/D'
                            if ((value == 0 && type == XOR) || (value == 1 && type == XNOR)){ //D(xor)
                                if (D_Dbar_count_tot == 0 || D_odd == 0){
                                    if (odd1 == 0){
                                        for(int i=0; i<unodes.size(); i++){
                                            if(unodes[i]->value==2) {
                                                // unodes[i]->D=1; 
                                                unodes[i]->value=1;
                                                break;
                                            }
                                        }   
                                    }
                                    else{ //odd1 != 0
                                        for(int i=0; i<unodes.size(); i++){
                                            if(unodes[i]->value==2) {
                                                // unodes[i]->D=0;
                                                unodes[i]->value=0;
                                                break;
                                            }
                                        }   
                                    }
                                }
                                else if (!D_Dbar_count_tot == 0 && D_odd == 1){
                                    if (odd1 == 0){
                                        for(int i=0; i<unodes.size(); i++){
                                            if(unodes[i]->value==2){
                                                if (D_more){
                                                    unodes[i]->value=0;
                                                    break;
                                                }
                                                else {
                                                    unodes[i]->value=1;
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                    else{ //odd1 != 0
                                        for(int i=0; i<unodes.size(); i++){
                                            if(unodes[i]->value==2){
                                                if (D_more){
                                                    unodes[i]->value=1;
                                                    break;
                                                }
                                                else {
                                                    unodes[i]->value=0;
                                                    break;
                                                }
                                            }
                                        }
                                    }  
                                }
                            }
                            else if ((value == 1 && type == XOR) || (value == 0 && type == XNOR)){ //D'(xor)
                                if (D_Dbar_count_tot == 0 || D_odd == 0){
                                    if (odd1 == 0){
                                        for(int i=0; i<unodes.size(); i++){
                                            if(unodes[i]->value==2) {
                                                // unodes[i]->D=0;
                                                unodes[i]->value=0;
                                                break;
                                            }
                                        }   
                                    }
                                    else{ //odd1 != 0
                                        for(int i=0; i<unodes.size(); i++){
                                            if(unodes[i]->value==2) {
                                                // unodes[i]->D=1;
                                                unodes[i]->value=1;
                                                break;
                                            }
                                        }   
                                    }
                                }
                                else if (!D_Dbar_count_tot == 0 && D_odd == 1){
                                    if (odd1 == 0){
                                        for(int i=0; i<unodes.size(); i++){
                                            if(unodes[i]->value==2){
                                                if (D_more){
                                                    unodes[i]->value=1;
                                                    break;
                                                }
                                                else {
                                                    unodes[i]->value=0;
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                    else{ //odd1 != 0
                                        for(int i=0; i<unodes.size(); i++){
                                            if(unodes[i]->value==2){
                                                if (D_more){
                                                    unodes[i]->value=0;
                                                    break;
                                                }
                                                else {
                                                    unodes[i]->value=1;
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    break;
        case OR:
        case NOR:
            if(value==(type == OR)? 0^I_am_fault:1^I_am_fault)
            {
                for(int i=0;i<unodes.size();i++)
                {
                    // if(unodes[i]->value==2)
                    // {
                        unodes[i]->value=0;
                        unodes[i]->D=-1;
                    // }
                }
            }
            else if(x_count==1&&((count0+D_Dbar_count[0]+D_Dbar_count[1])==(unodes.size()-x_count)))      //if only 1 x output and others input is not 1
            {
                for(int i=0;i<unodes.size();i++)
                {
                    if(unodes[i]->value==2) unodes[i]->value=1; 
                }                
            }            
            break;
        case AND:
        case NAND:
            if(value==(type == AND)? (1^I_am_fault):(0^I_am_fault))
            {
                for(int i=0;i<unodes.size();i++)
                {
                    // if(unodes[i]->value==2)
                    // {
                        unodes[i]->value=1;
                        unodes[i]->D=-1;
                    // }
                }
            }
            else if(x_count==1&&(count1+D_Dbar_count[0]+D_Dbar_count[1])==(unodes.size()-x_count))  //if only 1 x output and others input is not 0
            {
                for(int i=0;i<unodes.size();i++)
                {
                    if(unodes[i]->value==2) unodes[i]->value=0;
                }                
            }
            break;
    }
}
int node_class::Unodes_x_count()
{
    int xcount=0;
    for(int i=0;i<unodes.size();i++)
    {
        xcount += (unodes[i]->value == 2); 
    }
    return xcount;  
}
int node_class::Unodes_1_count()
{
    int count1=0;
    for(int i=0;i<unodes.size();i++)
    {
        if(unodes[i]->value==1&&unodes[i]->D==-1)
        {
            count1++;
        }
    }
    return count1;      
} 
int node_class::Unodes_0_count()
{
    int count0=0;
    for(int i=0;i<unodes.size();i++)
    {
        if(unodes[i]->value==0&&unodes[i]->D==-1)
        {
            count0++;
        }
    }
    return count0;       
}
vector<int> node_class::Unodes_D_Dbar_count() //[0]:D, [1]:D'
{
    vector<int> dcount={0,0};
    for(int i=0;i<unodes.size();i++)
    {
        if(unodes[i]->D==1) dcount[0]++;
        else if (unodes[i]->D==0) dcount[1]++;
    }
    return dcount;      
}
bool node_class::forward_check()
{
    int temp_value = value;
    int temp_D = D;
    //D = -1;     
    Simulate();
    if(temp_D>=0)
    {
        if(value==2)                   //for PODEM
        {
            // cout<<"the node is "<<num<<endl;
            D = -1;
            Simulate();
            if(temp_D==0&&value==1) D = -1;         //if the simluate cause the fault become not the fault, the D change back to -1
            else if(temp_D==1 &&value==0) D = -1; 
            else D = temp_D;        
            return true;
        }
        else if((temp_value==value))   //change
        {
            // cout<<"D and value not compatible"<<endl;
            return false;
        }

    }
    else
    {
        if(value!=temp_value||D!=temp_D) return false; //change
    }
    value = temp_value;
    D = temp_D;
    return true;
}

bool node_class::backward_check()
{
    int temp_value = value;
    int temp_D = D;
    vector<node_class> temp_unodes;
    temp_unodes.reserve(unodes.size());
    for(int i=0;i<unodes.size();i++)
    {
        temp_unodes[i].value = unodes[i]->value ;
        temp_unodes[i].D = unodes[i]->D;
    }
    backward_Simulate();
    // Simulate();
    for(int i=0;i<unodes.size();i++)
    {
        // cout<<"backward_sim "<<"num:"<<unodes[i]->num<<"\t value: "<<unodes[i]->value<<"\t D: "<<unodes[i]->D<<endl;
        if(temp_unodes[i].value!=2)
        {
            if(temp_unodes[i].value!=unodes[i]->value||temp_unodes[i].D!=unodes[i]->D)
            {
                value  = temp_value;
                D = temp_D;
                for(int i=0;i<unodes.size();i++)
                {
                    unodes[i]->value = temp_unodes[i].value;
                    unodes[i]->D = temp_unodes[i].D;
                }
                return false;
            }
        }
    }
    // if((value!=temp_value)&&value!=2) return false;
    value  = temp_value;
    D = temp_D;
    for(int i=0;i<unodes.size();i++)
    {
        unodes[i]->value = temp_unodes[i].value;
        unodes[i]->D = temp_unodes[i].D;
    }
    return true;
    
}
int node_class::CC0_CC1_calculation(){
    int temp0,temp1,min=0,min1,min0;
    vector<int>CC0_vector,CC1_vector;
    CC0_vector.clear();
    CC1_vector.clear();
    switch(type){ 
        case BRCH:
            CC0=unodes[0]->CC0;
            CC1=unodes[0]->CC1;
            break;
        case AND:
        case NAND:
            temp1=0;
            for(int i=0;i<unodes.size();i++){
                temp1+=unodes[i]->CC1;
                CC0_vector.push_back(unodes[i]->CC0);
            } 
            min=*min_element(CC0_vector.begin(),CC0_vector.end());
            CC0=(type==AND)?min+1:temp1+1;
            CC1=(type==AND)?temp1+1:min+1; 
            break;
        case OR:
        case NOR:
            temp0=0;
            for(int i=0;i<unodes.size();i++){
                temp0+=unodes[i]->CC0;
                CC1_vector.push_back(unodes[i]->CC1);
            }  
            min=*min_element(CC1_vector.begin(),CC1_vector.end());
            CC0=(type==OR)?temp0+1:min+1;
            CC1=(type==OR)?min+1:temp0+1;
            break;
        case XOR:
        case XNOR:
            temp0=0;
            temp1=0;
            for(int i=0;i<unodes.size();i++){
                for(int j=0;j<unodes.size();j++){
                    if(i!=j){
                        temp0+=unodes[j]->CC0;
                        temp1+=unodes[j]->CC1;
                    }
                }

                CC0_vector.push_back(temp0+unodes[i]->CC0);
                CC0_vector.push_back(temp1+unodes[i]->CC1);
                CC1_vector.push_back(temp0+unodes[i]->CC1);
                CC1_vector.push_back(temp1+unodes[i]->CC0);
            }
            min0=*min_element(CC0_vector.begin(), CC0_vector.end());
            min1=*min_element(CC1_vector.begin(), CC1_vector.end());
            CC0=(type==XOR)?min0+1:min1+1;
            CC1=(type==XOR)?min1+1:min0+1;
            break;
        case NOT:
        case BUF:
            CC0=(type==NOT)?unodes[0]->CC1+1:unodes[0]->CC0+1;
            CC1=(type==NOT)?unodes[0]->CC0+1:unodes[0]->CC1+1;
            break;
    }
    return CC0,CC1;
}
void node_class::CO_caculation(){
    vector<int>CO_vector;
    int temp0,temp1;
    CO_vector.clear();
    int a;
    switch(type){
        case BRCH:
            for(int i=0;i<unodes[0]->dnodes.size();i++){
                CO_vector.push_back(unodes[0]->dnodes[i]->CO);
            }
            unodes[0]->CO=*min_element(CO_vector.begin(),CO_vector.end());
            break;  
        case AND:
        case NAND:
            temp1=0;
            for(int i=0;i<unodes.size();i++){
                temp1+=unodes[i]->CC1;
            }
            for(int i=0;i<unodes.size();i++){
                unodes[i]->CO = temp1 - unodes[i]->CC1+1+CO;
            }
        /*
            for(int i=0;i<unodes.size();i++){
                temp1=0;
                for(int j=0;j<unodes.size();j++){
                    if(i!=j){
                        temp1+=unodes[j]->CC1;
                    }
                }
                unodes[i]->CO=temp1+1+CO;  
            }  */
            break;
        case OR:
        case NOR:
            temp0=0;
            for(int i=0;i<unodes.size();i++){
                temp0+=unodes[i]->CC0;
            }
            for(int i=0;i<unodes.size();i++){
                unodes[i]->CO = temp0 - unodes[i]->CC0 + 1 + CO;
            }
        /*
            for(int i=0;i<unodes.size();i++){
                temp0=0;
                for(int j=0;j<unodes.size();j++){
                    if(i!=j){
                        temp0+=unodes[j]->CC0;
                    }
                }
                unodes[i]->CO=temp0+1+CO;
            }  */
            break;
            case XOR:
            case XNOR:
                for(int i=0;i<unodes.size();i++){
                    for(int j=0;j<unodes.size();j++){
                        if(i!=j){
                            CO_vector.push_back(unodes[j]->CC0);
                            CO_vector.push_back(unodes[j]->CC1);
                        }
                    }
                    unodes[i]->CO=1+*min_element(CO_vector.begin(),CO_vector.end())+CO;
                }
                break;
        case NOT:
        case BUF:
            unodes[0]->CO=CO+1;
            break;
    }
}

class frontier
{
    public:
        // D-frontier: gates with X output and D/D' input
        unordered_set<node_class*> Dfrontier;
        // J-frontier: gates with non-X output but inputs don't justify output
        unordered_set<node_class*> Jfrontier;

        void updateDfrontier(node_class* node);
        void updateJfrontier(node_class* node);
        void frontierreset();
};

void frontier::updateDfrontier(node_class* node)
{
    bool hasDInput = false;
    if(node->value==2)
    {
        for(int i=0;i<node->unodes.size();i++)
        {
            if(node->unodes[i]->D>=0)
            {
                hasDInput=true;
                break;
            }
        }
    }
    if(hasDInput) Dfrontier.insert(node);
    else Dfrontier.erase(node);
}

void frontier::updateJfrontier(node_class* node)
{   
    int xCount = node->Unodes_x_count(); // if zero x input -> not a jfrontier
    bool I_am_fault=(node->D>=0);
    if(xCount == 0)
    {
        Jfrontier.erase(node);
        return;
    }

    if(node->value!=0 && node->value!=1)
    {
        Jfrontier.erase(node);
        return;
    }

    bool needJustification = false;

    switch(node->type)
    {
        case IPT:
            break;
        case BRCH:
            break;
        case NOT:
            break; 
        case BUF:
            break;   
        case AND:
        case NAND:
            if(node->value==((node->type==AND)? 0^I_am_fault:1^I_am_fault))
            {
                for(int i=0;i<node->unodes.size();i++)
                {
                    if(node->unodes[i]->value==0) // If any input is 0
                    {
                        needJustification=false;
                        break;
                    }
                }
                if(xCount>1)                    //if 3-input AND gate is: 1 1 x  and output is 0, it is not a j forntier, BUT I didn't update the x=0;
                {
                    needJustification=true;
                    break;
                }
            }
            break;
        case OR:
        case NOR:
            if(node->value==((node->type==OR)? 1^I_am_fault:0^I_am_fault))
            {
                for(int i=0;i<node->unodes.size();i++)
                {
                    if(node->unodes[i]->value==1) // If any input is X
                    {
                        needJustification=false;
                        break;
                    }
                }
                if(xCount>1)        //if 3-input AND gate is: 0 0 x  and output is 1, it is not a j forntier, BUT I didn't update the x=0;
                {
                    needJustification=true;
                    break;
                }
            }
            break;     
        case XOR:
        case XNOR:
            if(xCount>=1)
            {
                needJustification = true;
            }     
            break;
  
    }
    if(needJustification) {
        Jfrontier.insert(node);
    } else {
        Jfrontier.erase(node);
    }
}

void frontier::frontierreset()
{
    Dfrontier.clear();
    Jfrontier.clear();
}
/*----------------- Command definitions ----------------------------------*/
void cread(), pc(), help(), quit(),lev(),logicsim(),rtpg(), rfl(),dfs(),pfs(),tpfc(),scoap(),dalg(),podem(),tpg();
std::vector<cmdclass> command = 
{
   {"READ", cread, EXEC},
   {"PC", pc, CKTLD},
   {"HELP", help, EXEC},
   {"QUIT", quit, EXEC},
   {"LEV", lev, CKTLD},
   {"LOGICSIM",logicsim,CKTLD},
   {"RTPG",rtpg,CKTLD},
   {"RFL",rfl, CKTLD},
   {"DFS",dfs,CKTLD},
   {"PFS",pfs,CKTLD},
   {"TPFC",tpfc,CKTLD},
   {"SCOAP",scoap,CKTLD},
   {"DALG",dalg,CKTLD},
   {"PODEM",podem,CKTLD},
   {"TPG",tpg,CKTLD}
};

/*------------------------------------------------------------------------*/
enum e_state Gstate = EXEC;     /* global exectution sequence */

vector<node_class>Node;                    /* dynamic array of nodes */

vector<node_class *>Pinput;                /* pointer to array of primary inputs */

vector<node_class *>Poutput;               /* pointer to array of primary outputs */

vector<node_class *> level_sorted_node;    /* dynamic array of level sorted node*/
int Nnodes;                     /* number of nodes */
int Npi;                        /* number of primary inputs */
int Npo;                        /* number of primary outputs */
int Done = 0;                   /* status bit to terminate program */

string cp;                      /*the input string*/
stack<node_class *> my_stack;   /*doing inply and check*/
stack<node_class *> my_backtrack_stack; /*doing backward inply and check*/
map<string, string> params = {
    {"rtp",""},
    {"rtp_v0_fc",""},
    {"rtp_v1_fc",""},
    {"rtp_v2_fc",""},
    {"df",""},
    {"jf",""},
    {"fl",""},
    {"fo",""},
    {"alg",""}
};     /*for TPG store the hyperparameter*/
float fault_coverage; 
int rtpg_call_count = 0;          
/*----------------------------------vi--------------------------------------*/