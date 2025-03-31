#include "../inc/readckt.hpp"

using namespace std;

#define MAXLINE 10000               /* Input buffer size */
#define MAXNAME 10000               /* File name size */

/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: shell
description:
    This is the main program of the simulator. It displays the prompt, reads
    and parses the user command, and calls the corresponding routines.
    Commands not reconized by the parser are passed along to the shell.
    The command is executed according to some pre-determined sequence.
    For example, we have to read in the circuit description file before any
    action commands.  The code uses "Gstate" to check the execution
    sequence.
    Pointers to functions are used to make function calls which makes the
    code short and clean.
-----------------------------------------------------------------------*/
std::size_t strlen(const char* start) {
   const char* end = start;
   for( ; *end != '\0'; ++end)
      ;
   return end - start;
}

/*-----------------------------------------------------------------------
input: gate type
output: string of the gate type
called by: pc
description:
    The routine receive an integer gate type and return the gate type in
    character string.
-----------------------------------------------------------------------*/
std::string gname(int tp){
    switch(tp) {
        case 0: return("PI");
        case 1: return("BRANCH");
        case 2: return("XOR");
        case 3: return("OR");
        case 4: return("NOR");
        case 5: return("NOT");
        case 6: return("NAND");
        case 7: return("AND");
        case 8: return("XNOR");
        case 9: return ("BUF");
    }
    return "";
}

/*-----------------------------------------------------------------------
input: cp
output: vector of cp
called by: logicsim
description:
    Return the vector contain the path
-----------------------------------------------------------------------*/
std::vector<std::string> splitPaths(const std::string& input) 
{
    std::vector<std::string> paths;
    std::istringstream iss(input);
    std::string path;

    while (iss >> path) {
        paths.push_back(path);
    }

    return paths;
}

std::vector<node_class*> getSortedNodes(std::vector<node_class> &nodes) 
{
    std::vector<node_class*> sortedNodes;
    sortedNodes.reserve(Nnodes);
    
    for (auto& node : nodes) {
        sortedNodes.push_back(&node);
    }
    
    std::sort(sortedNodes.begin(), sortedNodes.end(),
              [](const node_class* a, const node_class* b) {
                  return a->level < b->level;
              });
    
    return sortedNodes;
}
std::vector<node_class*> getSortedNodes_reverse(std::vector<node_class> &nodes) 
{
    std::vector<node_class*> sortedNodes;
    sortedNodes.reserve(Nnodes);
    
    for (auto& node : nodes) {
        sortedNodes.push_back(&node);
    }
    
    std::sort(sortedNodes.begin(), sortedNodes.end(),
              [](const node_class* a, const node_class* b) {
                  return a->level > b->level;
              });
    
    return sortedNodes;
}
/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: main 
description:
    The routine prints ot help inormation for each command.
-----------------------------------------------------------------------*/
void help(){
    printf("READ filename - ");
    printf("read in circuit file and creat all data structures\n");
    printf("PC - ");
    printf("print circuit information\n");
    printf("HELP - ");
    printf("print this help information\n");
    printf("QUIT - ");
    printf("stop and exit\n");
}


/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: main 
description:
    Set Done to 1 which will terminates the program.
-----------------------------------------------------------------------*/
void quit(){
    Done = 1;
}

/*======================================================================*/

/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: cread
description:
    This routine clears the memory space occupied by the previous circuit
    before reading in new one. It frees up the dynamic arrays Node.unodes,
    Node.dnodes, Node.flist, Node, Pinput, Poutput, and Tap.
-----------------------------------------------------------------------*/
void clear(){
    int i;
    for(i = 0; i<Nnodes; i++) {
        Node[i].unodes.clear();
        // free(Node[i].unodes);

        Node[i].dnodes.clear();
        // free(Node[i].dnodes);
    }
    Node.clear();
    // free(Node);

    Pinput.clear();
    // free(Pinput);

    Poutput.clear();
    // free(Poutput);

    Gstate = EXEC;
}

/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: cread
description:
    This routine allocatess the memory space required by the circuit
    description data structure. It allocates the dynamic arrays Node,
    Node.flist, Node, Pinput, Poutput, and Tap. It also set the default
    tap selection and the fanin and fanout to 0.
-----------------------------------------------------------------------*/
void allocate(){
    int i;
    Node.resize(Nnodes);
    // vector<node_class> Node(Nnodes);
    // Node = (node_class *) malloc(Nnodes * sizeof(node_class));
    Pinput.resize(Npi);
    // vector<node_class *> Pinout(Npi);
    // Pinput = (node_class **) malloc(Npi * sizeof(node_class *));
    Poutput.resize(Npo);
    // vector<node_class *> Poutput(Npo);
    // Poutput = (node_class **) malloc(Npo * sizeof(node_class *));

    for(i = 0; i<Nnodes; i++) {
        Node[i].indx = i;
        Node[i].fin = Node[i].fout = 0;
    }
}


/*-----------------------------------------------------------------------
input: circuit description file name
output: nothing
called by: main
description:
    This routine reads in the circuit description file and set up all the
    required data structure. It first checks if the file exists, then it
    sets up a mapping table, determines the number of nodes, PI's and PO's,
    allocates dynamic data arrays, and fills in the structural information
    of the circuit. In the ISCAS circuit description format, only upstream
    nodes are specified. Downstream nodes are implied. However, to facilitate
    forward implication, they are also built up in the data structure.
    To have the maximal flexibility, three passes through the circuit file
    are required: the first pass to determine the size of the mapping table
    , the second to fill in the mapping table, and the third to actually
    set up the circuit information. These procedures may be simplified in
    the future.
-----------------------------------------------------------------------*/
std::string inp_name = "";
void cread(){
    char buf[MAXLINE];
    // int *tbl;
    int ntbl, i, j, k, nd, tp, fo, fi, ni = 0, no = 0;
    FILE *fd;
    node_class *np;
    std::ifstream file(cp);
    if((fd = fopen(cp.c_str(),"r")) == NULL){
        printf("File does not exist!\n");
        return;
    }
    std::cout<<cp<<std::endl;
    inp_name = cp;
    
    if(Gstate >= CKTLD) clear();
    Nnodes = Npi = Npo = ntbl = 0;
    while(fgets(buf, MAXLINE, fd) != NULL) {
        if(sscanf(buf,"%d %d", &tp, &nd) == 2) {
            if(ntbl < nd) ntbl = nd;
            Nnodes ++;
            if(tp == PI) Npi++;
            else if(tp == PO) Npo++;
        }
    }
    vector<int>tbl(++ntbl);
    // tbl = (int *) malloc(++ntbl * sizeof(int));
    
    fseek(fd, 0L, 0);
    i = 0;
    while(fgets(buf, MAXLINE, fd) != NULL) {
        if(sscanf(buf,"%d %d", &tp, &nd) == 2) tbl[nd] = i++;
    }
    
    allocate();
    fseek(fd, 0L, 0);
    while(fscanf(fd, "%d %d", &tp, &nd) != EOF) {
        np = &Node[tbl[nd]];
        // std::cout<<np<<endl;
        // std::cout<<"hi1"<<endl;
        np->num = nd;
        // std::cout<<"hi2"<<endl;
        if(tp == PI) Pinput[ni++] = np;
        else if(tp == PO) Poutput[no++] = np;
        // std::cout<<"hi3"<<endl;
        switch(tp) {
            case PI:
                np->ntype = PI;
                // break;
            case PO:
                np->ntype = PO;
                // break;
            case GATE:
                fscanf(fd, "%d %d %d", &np->type, &np->fout, &np->fin);
                np->ntype = GATE;
                break;
            case FB:
                np->fout = np->fin = 1;
                fscanf(fd, "%d", &np->type);
                np->ntype = FB;
                break;
            default:
                printf("Unknown node type!\n");
                exit(-1);
        }
        // std::cout<<"hi1"<<endl;
        np->unodes = vector<node_class*> (np->fin);
        // np->unodes = (node_class **) malloc(np->fin * sizeof(node_class *));
        np->dnodes = vector<node_class*> (np->fout);
        // np->dnodes = (node_class **) malloc(np->fout * sizeof(node_class *));
        // std::cout<<"hi2"<<endl;
        for(i = 0; i < np->fin; i++) {
            fscanf(fd, "%d", &nd);
            np->unodes[i] = &Node[tbl[nd]];
        }
        // std::cout<<"hi3"<<endl;
        for(i = 0; i < np->fout; np->dnodes[i++] = NULL);
    }
    for(i = 0; i < Nnodes; i++) {
        for(j = 0; j < Node[i].fin; j++) {
            np = Node[i].unodes[j];
            k = 0;
            while(np->dnodes[k] != NULL) k++;
            np->dnodes[k] = &Node[i];
        }
    }
    fclose(fd);
    Gstate = CKTLD;
    printf("==> OK");
}

/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: main
description:
    The routine prints out the circuit description from previous READ command.
-----------------------------------------------------------------------*/
void pc(){
    int i, j;
    node_class *np;
    std::string gname(int);
   
    printf(" Node   Type \tIn     \t\t\tOut    \n");
    printf("------ ------\t-------\t\t\t-------\n");
    for(i = 0; i<Nnodes; i++) {
        np = &Node[i];
        printf("\t\t\t\t\t");
        for(j = 0; j<np->fout; j++) printf("%d ",np->dnodes[j]->num);
        printf("\r%5d  %s\t", np->num, gname(np->type).c_str());
        for(j = 0; j<np->fin; j++) printf("%d ",np->unodes[j]->num);
        printf("\n");
    }
    printf("Primary inputs:  ");
    for(i = 0; i<Npi; i++) printf("%d ",Pinput[i]->num);
    printf("\n");
    printf("Primary outputs: ");
    for(i = 0; i<Npo; i++) printf("%d ",Poutput[i]->num);
    printf("\n\n");
    printf("Number of nodes = %d\n", Nnodes);
    printf("Number of primary inputs = %d\n", Npi);
    printf("Number of primary outputs = %d\n", Npo);
}

/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: logicsim, rtpg
description:
    sort the vector declared by node_class with the element num
-----------------------------------------------------------------------*/
bool sort_num_input(const node_class* a, const node_class* b){
    return a->num < b->num;
}

/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: 
description:
    sort the vector declared by node_class with the element level
-----------------------------------------------------------------------*/

std::vector<node_class*> sort_level_input(std::vector<node_class> &nodes) 
{
    std::vector<node_class*> sortedNodes;
    sortedNodes.reserve(Nnodes);
    
    for (auto& node : nodes) {
        sortedNodes.push_back(&node);
    }
    
    std::sort(sortedNodes.begin(), sortedNodes.end(),
              [](const node_class* a, const node_class* b) {
                  return a->level < b->level;
              });
    
    return sortedNodes;
}


/*-----------------------------------------------------------------------
input: circuit description file name
output: levelized circuit description file
called by: main
description:
    Levelized the circuits
-----------------------------------------------------------------------*/
void lev() 
{
    node_class *np;
    FILE *fd;
    if((fd = fopen(cp.c_str(),"w")) == NULL){
        printf("File does not exist!\n");
        return;
    }

    //find the filename
    int iPos = inp_name.find_last_of("/")+1;
    inp_name = inp_name.substr(iPos,inp_name.length()-iPos);
    inp_name = inp_name.substr(0,inp_name.rfind(".")); 

   // 初始化
    int G_num = 0;
    queue<node_class*> q; // 使用佇列來處理節點

    // 第一步:初始化所有節點level為-1,並找出所有PI節點
    for(int i = 0; i < Nnodes; i++) {
        np = &Node[i];
        np->level = -1;
        
        if(np->type == 0) { // PI節點
            np->level = 0;
            q.push(np); // PI節點加入佇列
        }
        
        if(np->type != 0 && np->type != 1) {
            G_num++;
        }
    }

    // 第二步:用BFS計算level
    while(!q.empty()) {
        np = q.front();
        q.pop();
        
        // 處理所有fanout節點
        for(int i = 0; i < np->fout; i++) {
            node_class* next = np->dnodes[i];
            
            // 檢查該節點的所有輸入是否都有level了
            bool all_inputs_ready = true;
            int max_input_level = -1;
            
            for(int j = 0; j < next->fin; j++) {
                if(next->unodes[j]->level == -1) {
                    all_inputs_ready = false;
                    break;
                }
                max_input_level = max(max_input_level, next->unodes[j]->level);
            }
            
            // 如果所有輸入都準備好且尚未處理過此節點
            if(all_inputs_ready && next->level == -1) {
                next->level = max_input_level + 1;
                q.push(next); // 加入佇列以處理它的fanout
            }
        }
    }
    fprintf(fd,"%s\n",inp_name.c_str());
    fprintf(fd,"#PI: %d\n",Npi);
    fprintf(fd,"#PO: %d\n",Npo);
    fprintf(fd,"#Nodes: %d\n",Nnodes);
    fprintf(fd,"#Gates: %d\n",G_num);
    for(int i=0;i<Nnodes;i++)
    {
        np=&Node[i];
        fprintf(fd,"%d %d\n",np->num,np->level);
    }
    std::cout<<"==> OK"<<std::endl;
    fclose(fd);
}
void lev_2()
{
    node_class *np;
   // 初始化
    int G_num = 0;
    queue<node_class*> q; // 使用佇列來處理節點

    // 第一步:初始化所有節點level為-1,並找出所有PI節點
    for(int i = 0; i < Nnodes; i++) {
        np = &Node[i];
        np->level = -1;
        
        if(np->type == 0) { // PI節點
            np->level = 0;
            q.push(np); // PI節點加入佇列
        }
        
        if(np->type != 0 && np->type != 1) {
            G_num++;
        }
    }

    // 第二步:用BFS計算level
    while(!q.empty()) {
        np = q.front();
        q.pop();
        
        // 處理所有fanout節點
        for(int i = 0; i < np->fout; i++) {
            node_class* next = np->dnodes[i];
            
            // 檢查該節點的所有輸入是否都有level了
            bool all_inputs_ready = true;
            int max_input_level = -1;
            
            for(int j = 0; j < next->fin; j++) {
                if(next->unodes[j]->level == -1) {
                    all_inputs_ready = false;
                    break;
                }
                max_input_level = max(max_input_level, next->unodes[j]->level);
            }
            
            // 如果所有輸入都準備好且尚未處理過此節點
            if(all_inputs_ready && next->level == -1) {
                next->level = max_input_level + 1;
                q.push(next); // 加入佇列以處理它的fanout
            }
        }
    }
}

/*===========================logicsim=================================*/
void logicsim()
{
    lev_2();
    static bool first_time = true;
    node_class *np;
    // std::vector<node_class*> level_sorted_node;
    string input_vector,result1="",result2="";

    //get the arg
    std::vector<std::string> paths = splitPaths(cp);    //paths[0]= first arg, paths[1] = second arg

    ifstream input_file(paths[0]);

    if(!input_file.is_open()){
        cerr<<"Cannot open a input file to do logicsim"<<endl;
        return;
    }

    ofstream output_file(paths[1]);
    if(!output_file.is_open()){
        cerr<<"Cannot open a input file to do logicsim"<<endl;
        return;
    }    

    //write Primary output pin to output file
    for(int i=0;i<Npo;i++)
    {
        np = Poutput[i];
        result1 += std::to_string(np->num)+",";
    }
    result1.pop_back(); //erase the last comma
    output_file<<result1<<endl;

    //sort pinput
    static vector<node_class*> sorted_Pins(Pinput.begin(), Pinput.end());


    //sort level
    static std::vector<node_class*> level_sorted_node;
    if(first_time)
    {
        sorted_Pins = vector<node_class*>(Pinput.begin(), Pinput.end());
        std::sort(sorted_Pins.begin(), sorted_Pins.end(), sort_num_input);
        level_sorted_node = sort_level_input(Node);
        first_time = false;
    }                    
    getline(input_file,input_vector);//pass the first line

    while(getline(input_file,input_vector))
    {   
        result1="",result2="";
        //input the data from tp file
        istringstream vectordata(input_vector);
        string input_value;
        int input_value_int;
        int i=0;
        while(getline(vectordata,input_value,','))
        {   
            if(input_value=="x") input_value = "2"; 
            input_value_int = stoi(input_value);
            sorted_Pins[i]->value=input_value_int;
            i++;
        }

        for (int i=0;i<level_sorted_node.size();i++)
        {
            np = level_sorted_node[i];
            //cout<<"num: "<<np->num<<", value: "<<np->value<<endl;
            if(!np->unodes.empty()) np->Simulate();
        }
        /*for(int i=0;i<Nnodes;i++)
        {
            np = &Node[i];
            cout<<"Node: "<<np->num<<", value: "<<np->value<<endl;
        }*/
        
        //write output data to the output file
        for(int i=0;i<Npo;i++)
        {
            np = Poutput[i];
            result2 += std::to_string(np->value)+",";
        }
        result2.pop_back();
        std::replace(result2.begin(), result2.end(), '2', 'x'); //replace all 2 -> x
        output_file<<result2<<endl;
    }
    input_file.close();
    output_file.close();
    cout<<"==> OK"<<endl;
}

/*-----------------------------------------------------------------------
input: #of test pattern, mode, output_file_name
output: rtpg
called by: main
description:
    random test pattern generation
-----------------------------------------------------------------------*/
void rtpg(){
    stringstream ss(cp);
    int tp_count;
    string mode, output_Filename;
    string result = "";
    ss>>tp_count>>mode>>output_Filename;
    ofstream rtpg_out;
    if(rtpg_call_count==0) rtpg_out.open(output_Filename);
    else rtpg_out.open(output_Filename,ios::app);
    if(!rtpg_out){
         cerr<<"Cannot open a output file to write RTPG"<<endl;
        return;
    }
    // cout<<"We will genereate "<<tp_count<<" patterns in "<<mode<<" mode"<<endl;
    //sort PI_ID
    if(rtpg_call_count==0)
    {
        vector<node_class*> sorted_Pins(Pinput.begin(), Pinput.end());
        std::sort(sorted_Pins.begin(), sorted_Pins.end(), sort_num_input);
        for(int z=0;z<Npi;z++)
        {
            result += std::to_string(sorted_Pins[z]->num)+",";
        }
        result.pop_back(); //erase the last comma
        rtpg_out<<result<<endl;
        sorted_Pins.clear();
    }
    srand(time(NULL));
    for(int i=0;i<tp_count;i++){
        result = "";
        for(int j=0;j<Npi;j++){
            if(mode=="b")
            {
                result +=std::to_string(rand()%2)+",";
            }
            if(mode=="t")
            {
                result +=std::to_string(rand()%3)+",";
            }
        }
        if(mode =="t") std::replace(result.begin(), result.end(), '2', 'x'); //replace all 2 -> x
        result.pop_back(); //erase the last comma
        rtpg_out<<result<<endl;
    }    
    rtpg_out.close();
    rtpg_call_count++;
    
    /*=====================Testing Outputfile in RTPG============*/
    ifstream in_file(output_Filename);
    if(!in_file){
        cerr<<"Cannot open the RTPG file to see the result"<<endl;
        return;
    }
    // string line;
    // while(getline(in_file,line)){
    //     cout<<line<<endl;
    // }
    in_file.close();
}

/*========================= Reduced Fault List ========================*/
void rfl(){
    node_class *np;
    string output_FileName;
    stringstream ss(cp);
    ss>>output_FileName;
    ofstream out_file(output_FileName);
    for(int i=0;i<Nnodes;i++){
        np=&Node[i];
        if(np->type==IPT||np->type==BRCH){
            out_file<<np->num<<"@0"<<endl;
            out_file<<np->num<<"@1"<<endl;
        }
    }
    out_file.close();
    /*==================== Test outfile in RCL ===============*/
    ifstream in_file(output_FileName);
    if(!in_file){
        cerr<<"Cannot open RCL outputFile"<<endl;
        return;
    }
    string line;
    while(getline(in_file,line)){
        cout<<line<<endl;
    }
    in_file.close();
}

/*========================= ALL Fault List ========================*/
void fl()
{
    node_class *np;
    string output_FileName;
    stringstream ss(cp);
    ss>>output_FileName;
    ofstream out_file(output_FileName);
    for(int i=0;i<Nnodes;i++){
        np=&Node[i];
        out_file<<np->num<<"@0"<<endl;
        out_file<<np->num<<"@1"<<endl;
    }
    out_file.close();    
}

/*========================= Reset the circuits ========================*/
void circuits_value_reset()
{
    node_class *np;
    for(int i=0;i<Nnodes;i++)
    {
        np=&Node[i];
        np->value=-1;
        np->D = -1;
    }  
}
/*========================= Deductive Fault Simulation ========================*/
class final_fault{
    public:
        int ssid;
        int ssvalue;
        final_fault(int ssid, int ssvalue) : ssid(ssid), ssvalue(ssvalue) {}
};

vector<final_fault> final_fault_vector;
int final_idx;

void dfs(){
    lev_2();
    final_fault_vector.clear();
    final_idx = 0;
    //get the arg
    std::vector<std::string> paths = splitPaths(cp);    //paths[0]= first arg, paths[1] = second arg
    ifstream input_file(paths[0]);
    if(!input_file.is_open()){
        cerr<<"Cannot open a input file to do logicsim"<<endl;
        return;
    }

    ofstream output_file(paths[1]);
    if(!output_file.is_open()){
        cerr<<"Cannot open a input file to do logicsim"<<endl;
        return;
    }

    //sort pinput
    vector<node_class*> sorted_Pins(Pinput.begin(), Pinput.end());
    std::sort(sorted_Pins.begin(), sorted_Pins.end(), sort_num_input);

    //sort level
    std::vector<node_class*> level_sorted_node = sort_level_input(Node);

    string in_line;    
    vector<int> PIval;
    node_class *np; 
    //read 1st line (nodeID)

    //run dfs
    unordered_map<int, int> FaultList;
    vector<int> temp;
    vector<vector<int>> ctr_set;
    getline(input_file, in_line);
    
    while (getline(input_file, in_line)) {
        istringstream ss(in_line);
        string temp_val;
        int ii=0;
        PIval.clear();
        while (getline(ss, temp_val, ',')) {
            sorted_Pins[ii]->value = stoi(temp_val);
            ii++;
        }

        //run logicsim
        for (int i=0;i<level_sorted_node.size();i++)
        {
            np = level_sorted_node[i];
            if(!np->unodes.empty()) np->Simulate();
        }
        /*for(int i=0;i<Nnodes;i++)
        {
            np = level_sorted_node[i];
            cout<<"num: "<<np->num<<", value: "<<np->value<<endl;
        }*/


        int ctr = 0;
        for (int i=0; i<Nnodes; i++){
            np = level_sorted_node[i];

            switch (np->type){
                case IPT:
                    if (np->value == 1){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 0;
                    }
                    else if (np->value == 0){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 1;
                    }
                    break;

                case BRCH:
                    for (int j=0; j<np->unodes[0]->dfs_detect.size(); j++){
                            np->dfs_detect.push_back(np->unodes[0]->dfs_detect[j]);
                    }
                    if (np->value == 1){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 0;
                    }
                    else if (np->value == 0){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 1;
                    }
                    break;

                case XOR:                
                    for (int x=0; x<np->fin; x++){
                        for (int y=0; y<np->unodes[x]->dfs_detect.size(); y++){
                            auto ff = find(np->dfs_detect.begin(), np->dfs_detect.end(), np->unodes[x]->dfs_detect[y]);
                            if (ff == np->dfs_detect.end()){
                                np->dfs_detect.push_back(np->unodes[x]->dfs_detect[y]);
                            }
                            else {
                                np->dfs_detect.erase(ff);
                            }
                        }
                    }

                    if (np->value == 1){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 0;
                    }
                    else if (np->value == 0){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 1;
                    }
                    break;  

                case OR:
                    ctr = 0;
                    for (int j=0; j<np->fin; j++){
                        if (np->unodes[j]->value == 1){
                            ctr ++;
                        }
                    }

                    if (ctr == 0){ //all gate inputs hold non-controlling value
                        for (int x=0; x<np->fin; x++){
                            for (int y=0; y<np->unodes[x]->dfs_detect.size(); y++){
                                if (find(np->dfs_detect.begin(), np->dfs_detect.end(), np->unodes[x]->dfs_detect[y]) == np->dfs_detect.end()){
                                    np->dfs_detect.push_back(np->unodes[x]->dfs_detect[y]);
                                }
                            }
                        }
                    }
                    else{ //at least one input holds controlling value
                        ctr_set.resize(ctr);
                        int ctr_idx=0;
                        for (int x=0; x<np->fin; x++){
                            if (np->unodes[x]->value == 1)
                            {
                                for (int y=0; y<np->unodes[x]->dfs_detect.size(); y++){
                                    ctr_set[ctr_idx].push_back(np->unodes[x]->dfs_detect[y]);
                                }
                            ctr_idx++;
                            }

                        }

                        if (!ctr_set.empty()){
                            for (int elem: ctr_set[0]) {
                                bool is_common = true;  
                                for (int j=1; j<ctr_set.size(); j++) {
                                    if (find(ctr_set[j].begin(), ctr_set[j].end(), elem) == ctr_set[j].end()){
                                        is_common = false;
                                        break;
                                    }
                                }

                                if (is_common) {
                                    temp.push_back(elem);
                                }
                            }
                        }
                        //temp = intersection of ctrI

                        for (int x=0; x<np->fin; x++){
                            if (np->unodes[x]->value == 0){
                                temp.erase(std::remove_if(temp.begin(), temp.end(), [&](int elem) {
                                    return std::find(np->unodes[x]->dfs_detect.begin(), np->unodes[x]->dfs_detect.end(), elem) != np->unodes[x]->dfs_detect.end();
                                }), temp.end());
                            }
                        }
                        //temp = intersection of ctrI - untion of nonctrI

                        for (int j=0; j<temp.size(); j++){
                            np->dfs_detect.push_back(temp[j]);
                        }
                    
                    }
                    if (np->value == 1){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 0;
                    }
                    else if (np->value == 0){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 1;
                    }
                    break;

                case NOR:
                    ctr = 0;
                    for (int j=0; j<np->fin; j++){
                        if (np->unodes[j]->value == 1){
                            ctr ++;
                        }
                    }

                    if (ctr == 0){ //all gate inputs hold non-controlling value
                        for (int x=0; x<np->fin; x++){
                            for (int y=0; y<np->unodes[x]->dfs_detect.size(); y++){
                                if (find(np->dfs_detect.begin(), np->dfs_detect.end(), np->unodes[x]->dfs_detect[y]) == np->dfs_detect.end()){
                                    np->dfs_detect.push_back(np->unodes[x]->dfs_detect[y]);
                                }
                            }
                        }
                    }
                    else{ //at least one input holds controlling value
                        ctr_set.resize(ctr);
                        int ctr_idx=0;
                        for (int x=0; x<np->fin; x++){
                            if (np->unodes[x]->value == 1)
                            {
                                for (int y=0; y<np->unodes[x]->dfs_detect.size(); y++){
                                    ctr_set[ctr_idx].push_back(np->unodes[x]->dfs_detect[y]);
                                }
                            ctr_idx++;
                            }

                        }

                        if (!ctr_set.empty()){
                            for (int elem: ctr_set[0]) {
                                bool is_common = true;  
                                for (int j=1; j<ctr_set.size(); j++) {
                                    if (find(ctr_set[j].begin(), ctr_set[j].end(), elem) == ctr_set[j].end()){
                                        is_common = false;
                                        break;
                                    }
                                }

                                if (is_common) {
                                    temp.push_back(elem);
                                }
                            }
                        }
                        //temp = intersection of ctrI

                        for (int x=0; x<np->fin; x++){
                            if (np->unodes[x]->value == 0){
                                temp.erase(std::remove_if(temp.begin(), temp.end(), [&](int elem) {
                                    return std::find(np->unodes[x]->dfs_detect.begin(), np->unodes[x]->dfs_detect.end(), elem) != np->unodes[x]->dfs_detect.end();
                                }), temp.end());
                            }
                        }
                        //temp = intersection of ctrI - untion of nonctrI

                        for (int j=0; j<temp.size(); j++){
                            np->dfs_detect.push_back(temp[j]);
                        }
                    
                    }
                    if (np->value == 1){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 0;
                    }
                    else if (np->value == 0){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 1;
                    }
                    break;

                case NOT:
                    for (int j=0; j<np->unodes[0]->dfs_detect.size(); j++){
                            np->dfs_detect.push_back(np->unodes[0]->dfs_detect[j]);
                    }
                    if (np->value == 1){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 0;
                    }
                    else if (np->value == 0){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 1;
                    }
                    break;

                case NAND:
                    ctr = 0;
                    for (int j=0; j<np->fin; j++){
                        if (np->unodes[j]->value == 0){
                            ctr ++;
                        }
                    }

                    if (ctr == 0){ //all gate inputs hold non-controlling value
                        for (int x=0; x<np->fin; x++){
                            for (int y=0; y<np->unodes[x]->dfs_detect.size(); y++){
                                if (find(np->dfs_detect.begin(), np->dfs_detect.end(), np->unodes[x]->dfs_detect[y]) == np->dfs_detect.end()){
                                    np->dfs_detect.push_back(np->unodes[x]->dfs_detect[y]);
                                }
                            }
                        }
                    }
                    else{ //at least one input holds controlling value
                        ctr_set.resize(ctr);
                        int ctr_idx=0;
                        for (int x=0; x<np->fin; x++){
                            if (np->unodes[x]->value == 0)
                            {
                                for (int y=0; y<np->unodes[x]->dfs_detect.size(); y++){
                                    ctr_set[ctr_idx].push_back(np->unodes[x]->dfs_detect[y]);
                                }
                            ctr_idx++;
                            }

                        }

                        if (!ctr_set.empty()){
                            for (int elem: ctr_set[0]) {
                                bool is_common = true;  
                                for (int j=1; j<ctr_set.size(); j++) {
                                    if (find(ctr_set[j].begin(), ctr_set[j].end(), elem) == ctr_set[j].end()){
                                        is_common = false;
                                        break;
                                    }
                                }

                                if (is_common) {
                                    temp.push_back(elem);
                                }
                            }
                        }
                        //temp = intersection of ctrI

                        for (int x=0; x<np->fin; x++){
                            if (np->unodes[x]->value == 1){
                                temp.erase(std::remove_if(temp.begin(), temp.end(), [&](int elem) {
                                    return std::find(np->unodes[x]->dfs_detect.begin(), np->unodes[x]->dfs_detect.end(), elem) != np->unodes[x]->dfs_detect.end();
                                }), temp.end());
                            }
                        }
                        //temp = intersection of ctrI - untion of nonctrI

                        for (int j=0; j<temp.size(); j++){
                            np->dfs_detect.push_back(temp[j]);
                        }
                    
                    }
                    if (np->value == 1){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 0;
                    }
                    else if (np->value == 0){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 1;
                    }
                    break;

                case AND:
                    ctr = 0;
                    for (int j=0; j<np->fin; j++){
                        if (np->unodes[j]->value == 0){
                            ctr ++;
                        }
                    }

                    if (ctr == 0){ //all gate inputs hold non-controlling value
                        for (int x=0; x<np->fin; x++){
                            for (int y=0; y<np->unodes[x]->dfs_detect.size(); y++){
                                if (find(np->dfs_detect.begin(), np->dfs_detect.end(), np->unodes[x]->dfs_detect[y]) == np->dfs_detect.end()){
                                    np->dfs_detect.push_back(np->unodes[x]->dfs_detect[y]);
                                }
                            }
                        }
                    }
                    else{ //at least one input holds controlling value
                        ctr_set.resize(ctr);
                        int ctr_idx=0;
                        for (int x=0; x<np->fin; x++){
                            if (np->unodes[x]->value == 0)
                            {
                                for (int y=0; y<np->unodes[x]->dfs_detect.size(); y++){
                                    ctr_set[ctr_idx].push_back(np->unodes[x]->dfs_detect[y]);
                                }
                            ctr_idx++;
                            }

                        }

                        if (!ctr_set.empty()){
                            for (int elem: ctr_set[0]) {
                                bool is_common = true;  
                                for (int j=1; j<ctr_set.size(); j++) {
                                    if (find(ctr_set[j].begin(), ctr_set[j].end(), elem) == ctr_set[j].end()){
                                        is_common = false;
                                        break;
                                    }
                                }

                                if (is_common) {
                                    temp.push_back(elem);
                                }
                            }
                        }
                        //temp = intersection of ctrI

                        for (int x=0; x<np->fin; x++){
                            if (np->unodes[x]->value == 1){
                                temp.erase(std::remove_if(temp.begin(), temp.end(), [&](int elem) {
                                    return std::find(np->unodes[x]->dfs_detect.begin(), np->unodes[x]->dfs_detect.end(), elem) != np->unodes[x]->dfs_detect.end();
                                }), temp.end());
                            }
                        }
                        //temp = intersection of ctrI - untion of nonctrI

                        for (int j=0; j<temp.size(); j++){
                            np->dfs_detect.push_back(temp[j]);
                        }
                    
                    }
                    if (np->value == 1){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 0;
                    }
                    else if (np->value == 0){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 1;
                    }
                    break;

                case XNOR:
                    for (int x=0; x<np->fin; x++){
                        for (int y=0; y<np->unodes[x]->dfs_detect.size(); y++){
                            auto ff = find(np->dfs_detect.begin(), np->dfs_detect.end(), np->unodes[x]->dfs_detect[y]);
                            if (ff == np->dfs_detect.end()){
                                np->dfs_detect.push_back(np->unodes[x]->dfs_detect[y]);
                            }
                            else {
                                np->dfs_detect.erase(ff);
                            }
                        }
                    }

                    if (np->value == 1){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 0;
                    }
                    else if (np->value == 0){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 1;
                    }
                    break;      

                case BUF:                        
                    for (int j=0; j<np->unodes[0]->dfs_detect.size(); j++){
                            np->dfs_detect.push_back(np->unodes[0]->dfs_detect[j]);
                    }
                    if (np->value == 1){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 0;
                    }
                    else if (np->value == 0){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 1;
                    }
                    break;
            }
            temp.clear();
            ctr_set.clear();
        }

        //check the detected ss@fault at POs 
        vector<int> dfs_po;
        for (int i=0; i<Npo; i++){
            np = Poutput[i];
            for (int j=0; j<np->dfs_detect.size(); j++){
                if (find(dfs_po.begin(), dfs_po.end(), np->dfs_detect[j]) == dfs_po.end()){
                    dfs_po.push_back(np->dfs_detect[j]);                
                }
            }
        }

        //output to txt
        for (int i=0; i<dfs_po.size(); i++){
            cout << dfs_po[i] << "@" << FaultList[dfs_po[i]] << endl;
        }
        
        bool found=false;
        for (int i=0; i<Npo; i++){
            np = Poutput[i];
            for (int j=0; j<np->dfs_detect.size(); j++){
                found = false;
                for (int k=0; k<final_fault_vector.size(); k++){
                    if (final_fault_vector[k].ssid == np->dfs_detect[j] && final_fault_vector[k].ssvalue == FaultList[np->dfs_detect[j]]){
                        found = true; 
                    }
                }
                if (found == false){
                    final_fault_vector.push_back({np->dfs_detect[j],FaultList[np->dfs_detect[j]]});
                }
            }
        }
        for (int s=0; s<Nnodes; s++){
            np = &Node[s];
            np->dfs_detect.clear();
        }
        FaultList.clear();
    }
    

    for (int i=0; i<final_fault_vector.size(); i++){
            output_file << final_fault_vector[i].ssid << "@" << final_fault_vector[i].ssvalue << endl;
    }

    input_file.close();
    output_file.close();
}

/*========================== Parallel Fault Simulation ==================*/
class Fault {
public:
    int node_id;
    int fault_type;
    bool detected;
    Fault(int id, int type) : node_id(id), fault_type(type), detected(false) {}
};
vector<Fault>fault_list;
static vector<node_class*> sorted_Pins(Pinput.begin(), Pinput.end());
static std::vector<node_class*> level_sorted_node_logic;
vector<int> PO_good_value;
vector<int> PO_fault_value;

//Functions
void read_fault_list(string& input_fl_file) {
    //Open Fault List File
    ifstream in_faultList_file(input_fl_file);
    if (!in_faultList_file) {
        cerr << "Cannot Open Fault List Input File" << endl;
        return;
    }

    string line;
    while (getline(in_faultList_file, line)) {
        stringstream ss(line);
        int node_id, Type;
        string data;
        getline(ss, data, '@');
        stringstream(data) >> node_id;
        getline(ss, data);
        stringstream(data) >> Type;
        fault_list.push_back({node_id,Type});
    }
    in_faultList_file.close();
}

void logicsim_simulation(string&line, bool &first_time){
    node_class *np;
    string input_vector,result1="",result2="";
    PO_good_value.clear();                    
    istringstream vectordata(line);
    string input_value;
    int input_value_int;
    int i=0;
    while(getline(vectordata,input_value,','))
        {   
            if(input_value=="x") input_value = "2"; 
            input_value_int = stoi(input_value);
            sorted_Pins[i]->value=input_value_int;
            i++;
        }

        for (int i=0;i<level_sorted_node_logic.size();i++){
            np = level_sorted_node_logic[i];
            if(!np->unodes.empty()) np->Simulate();
        }
        if(first_time==true){
            for(int i=0;i<Npo;i++){
                PO_good_value.push_back(Poutput[i]->value);
            }
        }
        
}
void logicsim_with_fault(string& line, int id, int value ){
    node_class *np;
    PO_fault_value.clear();
    int level=-1;
             
    istringstream vectordata(line);
    string input_value;
    int input_value_int;
    int i=0;               
    while(getline(vectordata,input_value,',')){   
        if(input_value=="x") input_value = "2"; 
        input_value_int = stoi(input_value);
        sorted_Pins[i]->value=input_value_int;
        i++;
    }
    for (int i=0;i<level_sorted_node_logic.size();i++){
        np = level_sorted_node_logic[i];
        if(np->num==id){
            np->value= static_cast<int>(value);
            level=np->level;
        }
        if(level!=-1&&np->level>=level+1 && !np->unodes.empty()) np->Simulate();
    }

    for(int i=0;i<Npo;i++){
        PO_fault_value.push_back(Poutput[i]->value);
    }        
}
void pfs(){
    string input_tp_file, input_fl_file, output_fl_file;
    stringstream ss(cp);
    ss >> input_tp_file >> input_fl_file >> output_fl_file;
    ofstream out_faultList_file(output_fl_file);
    if (!out_faultList_file) {
        cerr << "Cannot Open Fault List Output File" << endl;
        return;
    }
    ifstream in_pattern(input_tp_file);
    if(!in_pattern){
        cerr<<"Cannot Open Test Pattern Input File"<<endl;
    }
    string line;
    getline(in_pattern, line);
    read_fault_list(input_fl_file);
    lev_2();
    static bool first_time = true;
    if(first_time){
        sorted_Pins = vector<node_class*>(Pinput.begin(), Pinput.end());
        std::sort(sorted_Pins.begin(), sorted_Pins.end(), sort_num_input);
        level_sorted_node_logic = sort_level_input(Node);
        first_time = false;
    }     
    // Process each test pattern
    while (getline(in_pattern, line)) {
       bool first_time_logicsim=true;
        logicsim_simulation(line, first_time_logicsim);
        for(size_t i=0;i<fault_list.size();i++){
            if(fault_list[i].detected) continue;// Fault Dropping
            first_time_logicsim=false;
            int id=0;
            int sa_value=0, correct_value=0;
            id=fault_list[i].node_id;
            sa_value=fault_list[i].fault_type;
            logicsim_with_fault(line, id, sa_value);
            logicsim_simulation(line, first_time_logicsim);
            for(int j=0;j<Npo;j++){
                if(PO_fault_value[j]!=PO_good_value[j]){
                    fault_list[i].detected=true;//Set this fault is found.
                    out_faultList_file<<fault_list[i].node_id<<"@"<< fault_list[i].fault_type<<endl;
                    break;
                }
            }
        } 
    } 
    fault_list.clear();
    out_faultList_file.close(); 
    in_pattern.close();  
}

/*========================= SCOAP =====================================*/
void scoap(){
    string out_filename;
    stringstream ss(cp);
    ss>>out_filename;
    ofstream scoap_out_file(out_filename);
    if(!scoap_out_file){
        cerr<<"Cannot open SCOAP output file to write"<<endl;
        return ;
    }
    //Assign CC0, CC1 to Pinput and CO to Poutput
    for(int i=0;i<Npi;i++){
        Pinput[i]->CC0=1;
        Pinput[i]->CC1=1;
    }
    for(int i=0;i<Npo;i++){
        Poutput[i]->CO=0;
    }
    node_class *np;
    std::vector<node_class*> level_sorted_node;
    std::vector<node_class*> level_sorted_node_reverse;
    lev_2();
    level_sorted_node = getSortedNodes(Node);
    level_sorted_node_reverse=getSortedNodes_reverse(Node);

    //Calculate CC0,CC1
    for (int i=0;i<level_sorted_node.size();i++)
    {
        np = &Node[i];
        if(!np->unodes.empty())np->CC0_CC1_calculation();
    }
    //Calculate CO
    for(int i=0;i<level_sorted_node_reverse.size();i++){
        np=level_sorted_node_reverse[i];
        if(!np->unodes.empty())np->CO_caculation();
        // cout<<"The level from big to small is "<<level_sorted_node_reverse[i]->level<<", NO. "<<level_sorted_node_reverse[i]->num<<endl;
    }
    for(int i=0;i<Nnodes;i++){
        node_class *np;
        np=&Node[i];
        scoap_out_file<<np->num<<","<<np->CC0<<","<<np->CC1<<","<<np->CO<<endl;
        // cout<<np->num<<","<<np->CC0<<","<<np->CC1<<","<<np->CO<<endl;
    }
    scoap_out_file.close();
}
/*========================== Fault Coverage ===========================*/ 
void rtpg_FC(string tp_out_file,int tp_num){
    string result = "";
    ofstream rtpg_out(tp_out_file);
    if(!rtpg_out){
         cerr<<"Cannot open a output file to write RTPG"<<endl;
        return;
    }
    //sort PI_ID
    vector<node_class*> sorted_Pins(Pinput.begin(), Pinput.end());
    std::sort(sorted_Pins.begin(), sorted_Pins.end(), sort_num_input);
    for(int z=0;z<Npi;z++)
    {
        result += std::to_string(sorted_Pins[z]->num)+",";
    }
    result.pop_back(); //erase the last comma
    rtpg_out<<result<<endl;

    srand(time(NULL));
    for(int i=0;i<tp_num;i++){
        result = "";
        for(int j=0;j<Npi;j++){
            result +=std::to_string(rand()%2)+",";
        }
        result.pop_back(); //erase the last comma
        rtpg_out<<result<<endl;
    }    
    rtpg_out.close();
    sorted_Pins.clear();
    /* Test Result */
    ifstream in_file(tp_out_file);
    if(!in_file){
        cerr<<"Cannot open the RTPG file to see the result"<<endl;
        return;
    }
    string line;
    cout<<"RTPG Result!!!"<<endl;
    while(getline(in_file,line)){
        cout<<line<<endl;
    }
    in_file.close();
}  
vector<pair<int, int>> unique_faults_for_pattern;
void dfs_FC(string& line){
    lev_2();
    final_fault_vector.clear();
    final_idx = 0;
    //sort pinput
    vector<node_class*> sorted_Pins(Pinput.begin(), Pinput.end());
    std::sort(sorted_Pins.begin(), sorted_Pins.end(), sort_num_input);

    //sort level
    std::vector<node_class*> level_sorted_node = sort_level_input(Node);

    string in_line;    
    vector<int> PIval;
    node_class *np; 
    //read 1st line (nodeID)

    //run dfs
    unordered_map<int, int> FaultList;
    vector<int> temp;
    vector<vector<int>> ctr_set;
    
        istringstream ss(line);
        string temp_val;
        int ii=0;
        PIval.clear();
        while (getline(ss, temp_val, ',')) {
            sorted_Pins[ii]->value = stoi(temp_val);
            ii++;
        }

        //run logicsim
        for (int i=0;i<level_sorted_node.size();i++)
        {
            np = level_sorted_node[i];
            if(!np->unodes.empty()) np->Simulate();
        }
        /*for(int i=0;i<Nnodes;i++)
        {
            np = level_sorted_node[i];
            cout<<"num: "<<np->num<<", value: "<<np->value<<endl;
        }*/


        int ctr = 0;
        for (int i=0; i<Nnodes; i++){
            np = level_sorted_node[i];

            switch (np->type){
                case IPT:
                    if (np->value == 1){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 0;
                    }
                    else if (np->value == 0){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 1;
                    }
                    break;

                case BRCH:
                    for (int j=0; j<np->unodes[0]->dfs_detect.size(); j++){
                            np->dfs_detect.push_back(np->unodes[0]->dfs_detect[j]);
                    }
                    if (np->value == 1){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 0;
                    }
                    else if (np->value == 0){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 1;
                    }
                    break;

                case XOR:                
                    for (int x=0; x<np->fin; x++){
                        for (int y=0; y<np->unodes[x]->dfs_detect.size(); y++){
                            auto ff = find(np->dfs_detect.begin(), np->dfs_detect.end(), np->unodes[x]->dfs_detect[y]);
                            if (ff == np->dfs_detect.end()){
                                np->dfs_detect.push_back(np->unodes[x]->dfs_detect[y]);
                            }
                            else {
                                np->dfs_detect.erase(ff);
                            }
                        }
                    }

                    if (np->value == 1){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 0;
                    }
                    else if (np->value == 0){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 1;
                    }
                    break;

                case OR:
                    ctr = 0;
                    for (int j=0; j<np->fin; j++){
                        if (np->unodes[j]->value == 1){
                            ctr ++;
                        }
                    }

                    if (ctr == 0){ //all gate inputs hold non-controlling value
                        for (int x=0; x<np->fin; x++){
                            for (int y=0; y<np->unodes[x]->dfs_detect.size(); y++){
                                if (find(np->dfs_detect.begin(), np->dfs_detect.end(), np->unodes[x]->dfs_detect[y]) == np->dfs_detect.end()){
                                    np->dfs_detect.push_back(np->unodes[x]->dfs_detect[y]);
                                }
                            }
                        }
                    }
                    else{ //at least one input holds controlling value
                        ctr_set.resize(ctr);
                        int ctr_idx=0;
                        for (int x=0; x<np->fin; x++){
                            if (np->unodes[x]->value == 1)
                            {
                                for (int y=0; y<np->unodes[x]->dfs_detect.size(); y++){
                                    ctr_set[ctr_idx].push_back(np->unodes[x]->dfs_detect[y]);
                                }
                            ctr_idx++;
                            }

                        }

                        if (!ctr_set.empty()){
                            for (int elem: ctr_set[0]) {
                                bool is_common = true;  
                                for (int j=1; j<ctr_set.size(); j++) {
                                    if (find(ctr_set[j].begin(), ctr_set[j].end(), elem) == ctr_set[j].end()){
                                        is_common = false;
                                        break;
                                    }
                                }

                                if (is_common) {
                                    temp.push_back(elem);
                                }
                            }
                        }
                        //temp = intersection of ctrI

                        for (int x=0; x<np->fin; x++){
                            if (np->unodes[x]->value == 0){
                                temp.erase(std::remove_if(temp.begin(), temp.end(), [&](int elem) {
                                    return std::find(np->unodes[x]->dfs_detect.begin(), np->unodes[x]->dfs_detect.end(), elem) != np->unodes[x]->dfs_detect.end();
                                }), temp.end());
                            }
                        }
                        //temp = intersection of ctrI - untion of nonctrI

                        for (int j=0; j<temp.size(); j++){
                            np->dfs_detect.push_back(temp[j]);
                        }
                    
                    }
                    if (np->value == 1){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 0;
                    }
                    else if (np->value == 0){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 1;
                    }
                    break;

                case NOR:
                    ctr = 0;
                    for (int j=0; j<np->fin; j++){
                        if (np->unodes[j]->value == 1){
                            ctr ++;
                        }
                    }

                    if (ctr == 0){ //all gate inputs hold non-controlling value
                        for (int x=0; x<np->fin; x++){
                            for (int y=0; y<np->unodes[x]->dfs_detect.size(); y++){
                                if (find(np->dfs_detect.begin(), np->dfs_detect.end(), np->unodes[x]->dfs_detect[y]) == np->dfs_detect.end()){
                                    np->dfs_detect.push_back(np->unodes[x]->dfs_detect[y]);
                                }
                            }
                        }
                    }
                    else{ //at least one input holds controlling value
                        ctr_set.resize(ctr);
                        int ctr_idx=0;
                        for (int x=0; x<np->fin; x++){
                            if (np->unodes[x]->value == 1)
                            {
                                for (int y=0; y<np->unodes[x]->dfs_detect.size(); y++){
                                    ctr_set[ctr_idx].push_back(np->unodes[x]->dfs_detect[y]);
                                }
                            ctr_idx++;
                            }

                        }

                        if (!ctr_set.empty()){
                            for (int elem: ctr_set[0]) {
                                bool is_common = true;  
                                for (int j=1; j<ctr_set.size(); j++) {
                                    if (find(ctr_set[j].begin(), ctr_set[j].end(), elem) == ctr_set[j].end()){
                                        is_common = false;
                                        break;
                                    }
                                }

                                if (is_common) {
                                    temp.push_back(elem);
                                }
                            }
                        }
                        //temp = intersection of ctrI

                        for (int x=0; x<np->fin; x++){
                            if (np->unodes[x]->value == 0){
                                temp.erase(std::remove_if(temp.begin(), temp.end(), [&](int elem) {
                                    return std::find(np->unodes[x]->dfs_detect.begin(), np->unodes[x]->dfs_detect.end(), elem) != np->unodes[x]->dfs_detect.end();
                                }), temp.end());
                            }
                        }
                        //temp = intersection of ctrI - untion of nonctrI

                        for (int j=0; j<temp.size(); j++){
                            np->dfs_detect.push_back(temp[j]);
                        }
                    
                    }
                    if (np->value == 1){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 0;
                    }
                    else if (np->value == 0){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 1;
                    }
                    break;

                case NOT:
                    for (int j=0; j<np->unodes[0]->dfs_detect.size(); j++){
                            np->dfs_detect.push_back(np->unodes[0]->dfs_detect[j]);
                    }
                    if (np->value == 1){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 0;
                    }
                    else if (np->value == 0){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 1;
                    }
                    break;

                case NAND:
                    ctr = 0;
                    for (int j=0; j<np->fin; j++){
                        if (np->unodes[j]->value == 0){
                            ctr ++;
                        }
                    }

                    if (ctr == 0){ //all gate inputs hold non-controlling value
                        for (int x=0; x<np->fin; x++){
                            for (int y=0; y<np->unodes[x]->dfs_detect.size(); y++){
                                if (find(np->dfs_detect.begin(), np->dfs_detect.end(), np->unodes[x]->dfs_detect[y]) == np->dfs_detect.end()){
                                    np->dfs_detect.push_back(np->unodes[x]->dfs_detect[y]);
                                }
                            }
                        }
                    }
                    else{ //at least one input holds controlling value
                        ctr_set.resize(ctr);
                        int ctr_idx=0;
                        for (int x=0; x<np->fin; x++){
                            if (np->unodes[x]->value == 0)
                            {
                                for (int y=0; y<np->unodes[x]->dfs_detect.size(); y++){
                                    ctr_set[ctr_idx].push_back(np->unodes[x]->dfs_detect[y]);
                                }
                            ctr_idx++;
                            }

                        }

                        if (!ctr_set.empty()){
                            for (int elem: ctr_set[0]) {
                                bool is_common = true;  
                                for (int j=1; j<ctr_set.size(); j++) {
                                    if (find(ctr_set[j].begin(), ctr_set[j].end(), elem) == ctr_set[j].end()){
                                        is_common = false;
                                        break;
                                    }
                                }

                                if (is_common) {
                                    temp.push_back(elem);
                                }
                            }
                        }
                        //temp = intersection of ctrI

                        for (int x=0; x<np->fin; x++){
                            if (np->unodes[x]->value == 1){
                                temp.erase(std::remove_if(temp.begin(), temp.end(), [&](int elem) {
                                    return std::find(np->unodes[x]->dfs_detect.begin(), np->unodes[x]->dfs_detect.end(), elem) != np->unodes[x]->dfs_detect.end();
                                }), temp.end());
                            }
                        }
                        //temp = intersection of ctrI - untion of nonctrI

                        for (int j=0; j<temp.size(); j++){
                            np->dfs_detect.push_back(temp[j]);
                        }
                    
                    }
                    if (np->value == 1){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 0;
                    }
                    else if (np->value == 0){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 1;
                    }
                    break;

                case AND:
                    ctr = 0;
                    for (int j=0; j<np->fin; j++){
                        if (np->unodes[j]->value == 0){
                            ctr ++;
                        }
                    }

                    if (ctr == 0){ //all gate inputs hold non-controlling value
                        for (int x=0; x<np->fin; x++){
                            for (int y=0; y<np->unodes[x]->dfs_detect.size(); y++){
                                if (find(np->dfs_detect.begin(), np->dfs_detect.end(), np->unodes[x]->dfs_detect[y]) == np->dfs_detect.end()){
                                    np->dfs_detect.push_back(np->unodes[x]->dfs_detect[y]);
                                }
                            }
                        }
                    }
                    else{ //at least one input holds controlling value
                        ctr_set.resize(ctr);
                        int ctr_idx=0;
                        for (int x=0; x<np->fin; x++){
                            if (np->unodes[x]->value == 0)
                            {
                                for (int y=0; y<np->unodes[x]->dfs_detect.size(); y++){
                                    ctr_set[ctr_idx].push_back(np->unodes[x]->dfs_detect[y]);
                                }
                            ctr_idx++;
                            }

                        }

                        if (!ctr_set.empty()){
                            for (int elem: ctr_set[0]) {
                                bool is_common = true;  
                                for (int j=1; j<ctr_set.size(); j++) {
                                    if (find(ctr_set[j].begin(), ctr_set[j].end(), elem) == ctr_set[j].end()){
                                        is_common = false;
                                        break;
                                    }
                                }

                                if (is_common) {
                                    temp.push_back(elem);
                                }
                            }
                        }
                        //temp = intersection of ctrI

                        for (int x=0; x<np->fin; x++){
                            if (np->unodes[x]->value == 1){
                                temp.erase(std::remove_if(temp.begin(), temp.end(), [&](int elem) {
                                    return std::find(np->unodes[x]->dfs_detect.begin(), np->unodes[x]->dfs_detect.end(), elem) != np->unodes[x]->dfs_detect.end();
                                }), temp.end());
                            }
                        }
                        //temp = intersection of ctrI - untion of nonctrI

                        for (int j=0; j<temp.size(); j++){
                            np->dfs_detect.push_back(temp[j]);
                        }
                    
                    }
                    if (np->value == 1){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 0;
                    }
                    else if (np->value == 0){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 1;
                    }
                    break;

                case XNOR:
                    for (int x=0; x<np->fin; x++){
                        for (int y=0; y<np->unodes[x]->dfs_detect.size(); y++){
                            auto ff = find(np->dfs_detect.begin(), np->dfs_detect.end(), np->unodes[x]->dfs_detect[y]);
                            if (ff == np->dfs_detect.end()){
                                np->dfs_detect.push_back(np->unodes[x]->dfs_detect[y]);
                            }
                            else {
                                np->dfs_detect.erase(ff);
                            }
                        }
                    }

                    if (np->value == 1){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 0;
                    }
                    else if (np->value == 0){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 1;
                    }
                    break;

                case BUF:                        
                    for (int j=0; j<np->unodes[0]->dfs_detect.size(); j++){
                            np->dfs_detect.push_back(np->unodes[0]->dfs_detect[j]);
                    }
                    if (np->value == 1){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 0;
                    }
                    else if (np->value == 0){
                        np->dfs_detect.push_back(np->num);
                        FaultList[np->num] = 1;
                    }
                    break;
            }
            temp.clear();
            ctr_set.clear();
        }

        //check the detected ss@fault at POs 
        vector<int> dfs_po;
        for (int i=0; i<Npo; i++){
            np = Poutput[i];
            for (int j=0; j<np->dfs_detect.size(); j++){
                if (find(dfs_po.begin(), dfs_po.end(), np->dfs_detect[j]) == dfs_po.end()){
                    dfs_po.push_back(np->dfs_detect[j]);                
                }
            }
        }
        
        //output to txt
        // for (int i=0; i<dfs_po.size(); i++){
        //     cout << dfs_po[i] << "@" << FaultList[dfs_po[i]] << endl;
        // }
        
        bool found=false;
        for (int i=0; i<Npo; i++){
            np = Poutput[i];
            for (int j=0; j<np->dfs_detect.size(); j++){
                int fault_type = FaultList[j];
                
                found = false;
                for (int k=0; k<final_fault_vector.size(); k++){
                    if (final_fault_vector[k].ssid == np->dfs_detect[j] && final_fault_vector[k].ssvalue == FaultList[np->dfs_detect[j]]){
                        found = true; 
                    }
                }
                if (found == false){
                    final_fault_vector.push_back({np->dfs_detect[j],FaultList[np->dfs_detect[j]]});
                }
            }
        }
        for (int s=0; s<Nnodes; s++){
            np = &Node[s];
            np->dfs_detect.clear();
        }
        //Related to Fault Coverage 
        for(int i=0;i<final_fault_vector.size();i++){
            int fault_id = final_fault_vector[i].ssid;
            int fault_value = final_fault_vector[i].ssvalue;
            unique_faults_for_pattern.push_back({fault_id, fault_value});
        }
        std::sort(unique_faults_for_pattern.begin(), unique_faults_for_pattern.end());
        auto unique_end = std::unique(unique_faults_for_pattern.begin(), unique_faults_for_pattern.end());
        unique_faults_for_pattern.erase(unique_end, unique_faults_for_pattern.end());
        // Look Fault List
        // for(int i=0;i<unique_faults_for_pattern.size();i++){
        //     cout<<"The unique fault are "<<unique_faults_for_pattern[i].first<<"@"<<unique_faults_for_pattern[i].second<<endl;
        // }
        FaultList.clear();
}  
/*==================================== DTPFC ========================================*/
//DFS FC
void tpfc(){
    int freq;
    string input_tp_file,report_file;
    stringstream ss(cp);
    ss>>input_tp_file>>freq>>report_file;
    int pattern_counter=0;
    
    ofstream reprt_out_file(report_file);
    unique_faults_for_pattern.clear();
    if(!reprt_out_file){
        cerr<<"Cannot open Report output File"<<endl;
        return;
    }
    string line;
    ifstream in_tp_file(input_tp_file);
    float total_fault_count=0;
    reprt_out_file << std::fixed << std::setprecision(2);
    getline(in_tp_file,line);
    total_fault_count=2*Nnodes;
    // cout<<"Fault count is "<<total_fault_count<<endl;
    while(getline(in_tp_file,line)){
        dfs_FC(line);
        pattern_counter++;  
        if(pattern_counter%freq==0){
            // for(int i=0;i<unique_faults_for_pattern.size();i++){
            //     cout<<"The unique fault are "<<unique_faults_for_pattern[i].first<<"@"<<unique_faults_for_pattern[i].second<<endl;
            // }
            fault_coverage = (static_cast<float>(unique_faults_for_pattern.size())/total_fault_count)*100;
            reprt_out_file<<fault_coverage<<endl;
        }

    }
    reprt_out_file.close();
}

/*========================= DALG =====================================*/
bool imply_and_check(node_class *my_np,frontier & current_Frontier)
{
    node_class *current;
    // updatefrontier
    current_Frontier.updateDfrontier(my_np);
    current_Frontier.updateJfrontier(my_np); 

    static int h=0;
    for(const auto& dnode : my_np->dnodes) 
    {
        if(dnode->value == 2) {
            my_stack.push(dnode);
            // cout << "I push node:" << dnode->num << " into the stack" << endl;
        }
    }


    while(!my_stack.empty())
    {
        current = my_stack.top();
        // cout<<"current node num: "<<current->num<<endl;
        my_stack.pop();

        if(current->value!=2) continue;
        
        current->Simulate();

        // // updatefrontier
        current_Frontier.updateDfrontier(current);
        current_Frontier.updateJfrontier(current); 

        if(current->value==2)
        {
            continue;
        }
        //check conflict
        for(int i=0;i<current->dnodes.size();i++)
        {
            if(current->dnodes[i]->value!=2)
            {
                if(current->dnodes[i]->Unodes_x_count()==0)
                {
                    // cout<<"checking between "<<current->num<<" and "<<current->dnodes[i]->num<<endl;
                    current->dnodes[i]->D = -1;
                    if(!current->dnodes[i]->forward_check())
                    {
                        // cout << "forward Conflict detected between node " << current->num <<" and node "<<current->dnodes[i]->num<<endl;
                        return false; 
                    }
                    else
                    {
                        // if currect make sure to update current->dnodes[i] frontier because it cannot be update in other place
                        current_Frontier.updateDfrontier(current->dnodes[i]);
                        current_Frontier.updateJfrontier(current->dnodes[i]);             
                    }                    
                }
                else                                            //in case unode has x but can using backward to imply
                {
                    // cout<<"I push node:"<<current->dnodes[i]->num<<" into the backward stack"<<endl;
                    my_backtrack_stack.push(current->dnodes[i]);
                }
            }
        }

        for(int i=0;i<current->unodes.size();i++)                                                   //if backward and forward and another unode is x then backtrack!!
        {
            if(current->unodes[i]->value==2)
            {
                // cout<<"I push node:"<<current->num<<" into the backward stack"<<endl;
                my_backtrack_stack.push(current);
            }
        }
        for(int i=0;i<current->dnodes.size();i++)
        {
            if(current->value!=2)
            {
                if(!imply_and_check(current,current_Frontier))
                {
                    // cout<<"forward imply and check (recurtion) failed!!"<<endl;
                    return false;
                }
            }
        }
    }

    while(!my_backtrack_stack.empty())
    {
        current = my_backtrack_stack.top();
        // cout<<"start backward imply and my node num: "<<current->num<<", vaule: "<<current->value<<endl;
        my_backtrack_stack.pop();
        if(current->value==2) continue;
        if(current->Unodes_x_count()==0&&current->unodes.size()>0)
        {
            if(!current->backward_check())
            {
                // cout << "backward Conflict detected between node " << current->num <<" and node "<<current->unodes[0]->num<<endl;
                return false; 
            }            
        }
        if(current->Unodes_x_count()!=current->unodes.size())
        {
            for(int i = 0;i<current->unodes.size();i++)
            {
                // cout<<"num:"<<current->unodes[i]->num<<"\t value: "<<current->unodes[i]->value<<endl;
                if(current->unodes[i]->value!=2)
                {
                    //check conflict
                    if(!current->backward_check())
                    {
                        // cout << "backward Conflict detected between node " << current->num <<" and node "<<current->unodes[0]->num<<endl;
                        return false; 
                    }
                    // cout << "backward successful between node " << current->num <<" and node "<<current->unodes[0]->num<<endl;
                }
                else
                {
                    my_backtrack_stack.push(current->unodes[i]);
                    // cout<<"I push node:"<<current->unodes[i]->num<<" into the backward stack"<<endl;
                }         
            }
            current->backward_Simulate();
        }
        else
        {
            current->backward_Simulate();
            for(int i=0;i<current->unodes.size();i++)
            {
                if(current->unodes[i]->value!=2)
                {
                    my_backtrack_stack.push(current->unodes[i]);
                    // cout<<"I push node:"<<current->unodes[i]->num<<" into the backward stack"<<", value: "<<current->unodes[i]->value<<endl;
                }
            }
        }

        current_Frontier.updateDfrontier(current);
        current_Frontier.updateJfrontier(current);
        if(current->Dnodes_has_BRCH())
        {
            for(int i=0;i<current->dnodes.size();i++)
            {
                if(current->dnodes[i]->type==BRCH&&current->dnodes[i]->value==2)
                {
                    if(!imply_and_check(current,current_Frontier))
                    {
                        // cout<<"backward imply and check (recurtion) failed!!"<<endl;
                        return false;
                    }
                }
            }    
        }
        else
        {
            for(const auto& unode : current->unodes) 
            {
                if(unode->value != 2) 
                {
                    if(!imply_and_check(current, current_Frontier)) 
                    {
                        // cout << "backward imply and check (recurtion) failed!!" << endl;
                        return false;
                    }
                }
            }
        }
    }
    
    return true;

}
//Sort Nodes
bool sort_number_small(const node_class* a, node_class* b){
        return a->num < b->num;
}
bool sort_number_large(const node_class* a, node_class* b){
        return a->num > b->num;
}
bool sort_level_small(const node_class* a, node_class* b){
        return a->level < b->level;
}
bool sort_level_large (const node_class* a, node_class* b){
        return a->level > b->level;
}
bool sort_scoap(const node_class* a, node_class* b){
        return a->CO < b->CO;
}
bool my_dalg(node_class *current_np,frontier & current_Frontier)
{   
    lev_2();
    node_class* np;
    vector<node_class*>node_vector;
    vector<node_class*> sorted_vector;
    node_vector.clear();
    sorted_vector.clear();
    int c; //controlling value
    bool error_at_po_flag = false;
    static int count = 0;
    // cout<<"count: "<<count<<endl;
    count++;
    vector<node_class> temp_node;
    frontier temp_frontier;
    
    if(!imply_and_check(current_np,current_Frontier))
    {
        // cout<<"imply and check failed!!"<<endl;
        return false;
    }
    // cout<<"After imply and check"<<endl;
    // for(int i = 0; i < Nnodes; i++) 
    // {
    //     cout<<"num: "<<(&Node[i])->num<<"\tvalue: "<<(&Node[i])->value<<"\tD: "<<(&Node[i])->D<<endl;
    // }
    temp_frontier = current_Frontier;
    temp_node = Node;

    //check po has D or D'
    if(!error_at_po_flag)
    {
        for(int i=0;i<Npo;i++)
        {
            if(Poutput[i]->D>=0)
            {
                error_at_po_flag = true; 
                // cout<<"Node "<<Poutput[i]->num<<" got the error at output"<<endl;
            } 
        }
    }

    if(!error_at_po_flag)
    {
        if(temp_frontier.Dfrontier.size()==0)
        {
            // cout<<"Dfrontier is empty so its failed"<<endl;
            return false;
        } 
        //Update D Frontier
        //Collect D Frontier
        for(const auto& dfNode : temp_frontier.Dfrontier){
            node_vector.push_back(dfNode);
        }
        //Sorting 
        if(params["df"]=="nl"){
            std::sort(node_vector.begin(),node_vector.end(), sort_number_small);
            sorted_vector = node_vector;
        }
        else if(params["df"]=="nh"){
            std::sort(node_vector.begin(),node_vector.end(), sort_number_large);
            sorted_vector = node_vector;
        }
        else if(params["df"]=="lh"){
             std::sort(node_vector.begin(),node_vector.end(), sort_level_large);
             sorted_vector = node_vector;
        }
		else if(params["df"]=="ll"){
             std::sort(node_vector.begin(),node_vector.end(), sort_level_small);
             sorted_vector = node_vector;
        }
        else if(params["df"]=="cc"){
            std::sort(node_vector.begin(),node_vector.end(), sort_scoap);
            sorted_vector = node_vector;
        }
        else if(params["df"]==""){
            sorted_vector = node_vector;
        }
        for(const auto& dfNode : sorted_vector)
        {
            // for(auto itt = temp_frontier.Dfrontier.begin(); itt != temp_frontier.Dfrontier.end(); itt++)
            // {
            //     np = *itt;
            //     cout<<"The Dfrontier is "<<np->num<<endl;
            // }
            np = dfNode;
            // cout<<"I choose num: "<<np->num<<" as my Dfrontier"<<endl;
            // cout << "--------------------Before backtrack------------------------------" << endl;
            
            // for(int i = 0; i < Nnodes; i++) 
            // {
            //     cout<<"num: "<<(&Node[i])->num<<"\tvalue: "<<(&Node[i])->value<<"\tD: "<<(&Node[i])->D<<endl;
            // }
            if(np->type==3 ||np->type==4)       c = 1;//or-gate
            else if(np->type==6||np->type==7)   c = 0;//and-gate
            else if(np->type ==2||np->type==8)  c = 0;//xor-gate using 0 as controlling input better for propogate the falut
            c = !c;
            for(int i=0;i<np->unodes.size();i++)
            {
                if(np->unodes[i]->value==2)
                {
                    np->unodes[i]->value = c;
                    my_backtrack_stack.push(np->unodes[i]);                //wait for backtrack
                    // cout<<"adding num:"<<np->unodes[i]->num<<" to be "<<np->unodes[i]->value<<" to propogate the fault"<<endl;

                    current_Frontier.updateDfrontier(np);
                    current_Frontier.updateJfrontier(np);
                }
            }

            np->Simulate();
            // for(int i = 0; i < Nnodes; i++) 
            // {
            //     cout<<"num: "<<(&Node[i])->num<<"\tvalue: "<<(&Node[i])->value<<"\tD: "<<(&Node[i])->D<<endl;
            // }
            if(my_dalg(np,current_Frontier))
            {   
                // cout<<"np num: "<<np->num<<" return from my_dalgo(error not at PO) and success!!"<<endl;
                return true;
            }
            else
            {
                
                // cout<<"np num: "<<np->num<<" failed change another d frontier"<<endl;
                Node = temp_node;  
                if(np->type ==2||np->type==8)
                {
                    for(int i=0;i<np->unodes.size();i++)
                    {
                        if(np->unodes[i]->value==2)
                        {
                            np->unodes[i]->value = !c;
                            my_backtrack_stack.push(np->unodes[i]);                //wait for backtrack
                            // cout<<"adding num:"<<np->unodes[i]->num<<" to be "<<np->unodes[i]->value<<" to propogate the fault"<<endl;
                            current_Frontier.updateDfrontier(np);
                            current_Frontier.updateJfrontier(np);
                        }
                    }  
                    np->Simulate();
                    // for(int i = 0; i < Nnodes; i++) 
                    // {
                    //     cout<<"num: "<<(&Node[i])->num<<"\tvalue: "<<(&Node[i])->value<<"\tD: "<<(&Node[i])->D<<endl;
                    // }
                    if(my_dalg(np,current_Frontier))
                    {   
                        // cout<<"np num: "<<np->num<<" return from my_dalgo(error not at PO) and success!!"<<endl;
                        return true;
                    }                  
                }
                Node = temp_node; 
                // for(int i=0;i<Nnodes;i++)
                // {
                //     cout<<"restore the old num: "<<Node[i].num<<", value: "<<Node[i].value<<", D: "<<Node[i].D<<endl;
                // }
                //clear the backtrack stack
                while(!my_backtrack_stack.empty())
                {
                    my_backtrack_stack.pop();
                }                   
            }
        }
  
        cout<<"all d frontier is tried and failed!!"<<endl;
        return false;
    }
    if(error_at_po_flag)
    {
        if(temp_frontier.Jfrontier.empty())
        {
            // cout<<"J frontier is empty and successful!!!"<<endl;
            return true;
        }

        for(const auto& jfNode : temp_frontier.Jfrontier)
        {
            // for(auto itt = temp_frontier.Jfrontier.begin(); itt != temp_frontier.Jfrontier.end(); itt++)
            // {
            //     np = *itt;
            //     cout<<"The Jfrontier is "<<np->num<<endl;
            // }
            np = jfNode;
            bool u_node_has_x = (bool) np->Unodes_x_count();

            if(u_node_has_x)
            {
                if(np->type==3 ||np->type==4)       c = 1;//or-gate
                else if(np->type==6||np->type==7)   c = 0;//and-gate
                else if(np->type ==2||np->type==8)  c = 0;//xor-gate using 0 as controlling input better for propogate the falut
                
                // //advanced
                // if (params["jf"] == "v0"){
                //     node_class *lowCC_np;
                //     int lowCC_value = 100;
                //     if (c == 0){
                //         for (int i=0; i<np->unodes.size(); i++){
                //             if (np->unodes[i]->value == 2){
                //                 if (np->unodes[i]->CC0 < lowCC_value){
                //                     lowCC_np = np->unodes[i];
                //                     lowCC_value = np->unodes[i]->CC0;
                //                 }
                //             }
                //         }
                //     }
                //     else if (c == 1){
                //         for (int i=0; i<np->unodes.size(); i++){
                //             if (np->unodes[i]->value == 2){
                //                 if (np->unodes[i]->CC1 < lowCC_value){
                //                     lowCC_np = np->unodes[i];
                //                     lowCC_value = np->unodes[i]->CC1;
                //                 }
                //             }
                //         }
                //     }
                //     lowCC_np->value = c;
                //     my_backtrack_stack.push(lowCC_np);
                //     current_Frontier.updateDfrontier(np);
                //     current_Frontier.updateJfrontier(np);  
                //     if(my_dalg(np,current_Frontier))
                //     {
                //         // cout<<"np num:"<<np->num<<" return from my_dalgo(error at PO) and success!!"<<endl;
                //         return true;
                //     }
                //     else 
                //     {
                //         // cout<<"np num: "<<np->num<<" failed change another unode value of J frontier"<<endl;
                //         Node = temp_node;  
                //         lowCC_np->value = !c;
                //         // maybe?
                //         while(!my_backtrack_stack.empty())
                //         {
                //             my_backtrack_stack.pop();
                //         }
                //     }    
                // }
                // // //baseline 
                //     else{
                //         for(int i=0;i<np->unodes.size();i++)
                //         {
                //             if(np->unodes[i]->value==2)
                //             {   
                //                 np->unodes[i]->value = c;
                //                 my_backtrack_stack.push(np->unodes[i]);    
                //                 // cout<<"set jfrontier num:"<<np->num<<" unode num: "<<np->unodes[i]->num<<"into "<<c<<endl;
                //                 current_Frontier.updateDfrontier(np);
                //                 current_Frontier.updateJfrontier(np);  
                //                 if(my_dalg(np,current_Frontier))
                //                 {
                //                     // cout<<"np num:"<<np->num<<" return from my_dalgo(error at PO) and success!!"<<endl;
                //                     return true;
                //                 }
                //                     else 
                //                     {
                //                         // cout<<"np num: "<<np->num<<" failed change another unode value of J frontier"<<endl;
                //                         Node = temp_node;
                //                         if(!(np->type==2 || np->type==8)) np->unodes[i]->value = !c;
                //                         // maybe?
                //                         while(!my_backtrack_stack.empty())
                //                         {
                //                             my_backtrack_stack.pop();
                //                         }
                //                         while(!my_stack.empty())
                //                         {
                //                             my_stack.pop();
                //                         }
                //                         // for(int i=0;i<Nnodes;i++)
                //                         // {
                //                         //     cout<<"restore the old num: "<<Node[i].num<<", value: "<<Node[i].value<<", D: "<<Node[i].D<<endl;
                //                         // }                            
                //                     }
                //             }
                //         }
                //     }    
                        
                //baseline 
                for(int i=0;i<np->unodes.size();i++)
                {
                    if(np->unodes[i]->value==2)
                    {   
                        if((np->type==2 ||np->type==8)&&np->Unodes_x_count()==1)
                        {
                            np->backward_Simulate();
                        }
                        else
                        {
                            np->unodes[i]->value = c;
                        }
                        
                        my_backtrack_stack.push(np->unodes[i]);
                        // cout<<"set jfrontier num:"<<np->num<<" unode num: "<<np->unodes[i]->num<<" into "<<c<<endl;
                        current_Frontier.updateDfrontier(np);
                        current_Frontier.updateJfrontier(np);  
                        if(my_dalg(np,current_Frontier))
                        {
                            // cout<<"np num:"<<np->num<<" return from my_dalgo(error at PO) and success!!"<<endl;
                            return true;
                        }
                        else 
                        {
                            // cout<<"np num: "<<np->num<<" failed change another unode value of J frontier"<<endl;
                            Node = temp_node;
                            if(!(np->type==2 || np->type==8)) np->unodes[i]->value = !c;
                            // maybe?
                            while(!my_backtrack_stack.empty())
                            {
                                my_backtrack_stack.pop();
                            }
                            while(!my_stack.empty())
                            {
                                my_stack.pop();
                            }
                            // for(int i=0;i<Nnodes;i++)
                            // {
                            //     cout<<"restore the old num: "<<Node[i].num<<", value: "<<Node[i].value<<", D: "<<Node[i].D<<endl;
                            // }                            
                        }
                    }
                }
            }
        }
        // cout<<"all j frontier is tried and failed!!"<<endl;
        return false;
    }
    cout<<"weird place"<<endl;
    return true;
}

void dalg()
{
    node_class *np,*d_start_np;
    frontier current_Frontier;
    current_Frontier.frontierreset();
    stringstream ss(cp);
    unsigned Error_Node;
    int stuck_at;
    string path;
    ss>>Error_Node>>stuck_at>>path;
    ofstream output_file(path);
    string result1="",result2="";
    for(int i=0;i<Node.size();i++)
    {
        np = &Node[i];
        if (np->num==Error_Node)
        {
            d_start_np = np;
            np->value = stuck_at;
            np->D = (stuck_at==1)? 0:1;
            my_backtrack_stack.push(np);        //wait for backtrack
            current_Frontier.updateDfrontier(np);
            current_Frontier.updateJfrontier(np);
        }
        else
        {
            np->value = 2;
        }
    }

    if(!my_dalg(d_start_np,current_Frontier))
    {
        // cout<<"dalg failed!!"<<endl;
        for(int i=0;i<Pinput.size();i++)
        {
            np = Pinput[i];
            np->value = 2;
            np->D = -1;
        }
    }
    else
    {
    //   cout<<"dalg successful!!"<<endl;  
    }
    if(!output_file.is_open())
    {
        cerr<<"Cannot open a input file to do logicsim"<<endl;
        return;
    }
    for(int i=0;i<Pinput.size();i++)
    {
        np = Pinput[i];
        result1 += std::to_string(np->num)+",";
    }
    result1.pop_back();
    output_file<<result1<<endl;
    bool allTwo = true;
    result2 = "";
    for(int i = 0; i < Pinput.size(); i++) {
        np = Pinput[i];
        if(np->D >= 0) np->value = !np->value;
        if(np->value != 2) allTwo = false;
        result2 += std::to_string(np->value) + ",";
    }
    result2.pop_back();
    std::replace(result2.begin(), result2.end(), '2', 'x'); //replace all 2 -> x
    if(!allTwo) 
    {
        output_file<<result2<<endl;
    }
    output_file.close();
    circuits_value_reset();
}

/*========================= PODEM =====================================*/

pair<node_class*, int>  objective(node_class * np,int fault_value,frontier current_Frontier){
    // cout<<"target is "<<np->num<<", value is "<<np->value<<endl;
    //Exicite the fault
    vector<node_class*>node_vector;
    vector<node_class*> sorted_vector;
    node_vector.clear();
    sorted_vector.clear();
    for (const auto& dfNode : current_Frontier.Dfrontier){
            node_vector.push_back(dfNode);
        }
        if(params["df"]=="nl"){
            std::sort(node_vector.begin(),node_vector.end(), sort_number_small);
            sorted_vector = node_vector;
        }
        else if(params["df"]=="nh"){
            std::sort(node_vector.begin(),node_vector.end(), sort_number_large);
            sorted_vector = node_vector;
        }
        else if(params["df"]=="ll"){
             std::sort(node_vector.begin(),node_vector.end(), sort_level_small);
             sorted_vector = node_vector;
        }
		 else if(params["df"]=="lh"){
             std::sort(node_vector.begin(),node_vector.end(), sort_level_large);
             sorted_vector = node_vector;
        }
        else if(params["df"]=="cc"){
            std::sort(node_vector.begin(),node_vector.end(), sort_scoap);
            sorted_vector = node_vector;
        }
        else if(params["df"]==""){
            sorted_vector = node_vector;
        }
    if((np->value==2))
    {
        int vbar=!fault_value;
        // cout<<"target node is "<<np->num<<", vabr is "<<vbar<<endl;
        return {np, vbar};
    }
    else
    {
        // cout<<"obj in target node is "<<np->num<<", vabr is "<<np->value<<endl;
        //Select gate from D-forntier, so 
        // cout<<"We select D Frontier"<<endl;
        for (const auto& dfNode : sorted_vector)
        {
            for(const auto& itt : sorted_vector)
            {
                np = itt;
                // cout<<"I choose "<<np->num<<" as my Dfrontier"<<endl;
            }
            int c =0; //controlling value
            np = dfNode;
            // cout<<"the d frontier we choose is "<<np->num<<", value is "<<np->value<<endl;
            if(np->type==1)       c = 1;//branch
            else if(np->type==3 ||np->type==4)  c = 1;
            else if(np->type==6||np->type==7)   c = 0;//and-gate
            else if(np->type ==2||np->type==8)  c = 0;//xor-gate using 0 as controlling input better for propogate the falut
            c = !c; //non-controlling value
            for (int i=0; i<np->unodes.size(); i++)
            {
                if (np->unodes[i]->value ==2){
                    // cout<<"adding num:"<<np->unodes[i]->num<<" to be "<<c<<" to propogate the fault"<<endl;
                    return {np->unodes[i],c};
                }
            }
        }
    }
}
pair<node_class*, int> backtrace(node_class *k, int v_k){
    int v = v_k;
    bool inversion = false;
    
    if (k->type == NOT || k->type == NOR || k->type == NAND){
        inversion = true;
    }
    while (k->type != IPT){
        for (int i=0; i<k->fin; i++){
            if (k->unodes[i]->value == 2){
                k = k->unodes[i];
                break;
            }
        }
        if (k->type == NOT || k->type == NOR || k->type == NAND)
        {
            inversion = !inversion;
        }
    }

    if (inversion) v = !v;
    return {k, v};
}

void imply(node_class *j, int v_j,frontier &current_Frontier)
{
    if(j->D>=0&&j->type==IPT) j->value = !v_j;
    else j->value = v_j;
    // cout<<"the current imply node num is "<<j->num<<", value: "<<j->value<<", D: "<<j->D<<endl;
    for (int i=0; i<j->fout; i++)
    {
        if(j->dnodes[i]->value!=2)
        {
            imply(j->dnodes[i],j->dnodes[i]->value,current_Frontier);
            return;
        }
        else
        {
            j->dnodes[i]->Simulate();
        }
        // cout<<"after simulate node num is "<<j->dnodes[i]->num<<", value: "<<j->dnodes[i]->value<<", D: "<<j->dnodes[i]->D<<endl;
        if(j->dnodes[i]->D>=0&&j->dnodes[i]->value==2)
        {
            j->dnodes[i]->forward_check();
            if(j->dnodes[i]->value!=2)
            {
                j->dnodes[i]->value = j->dnodes[i]->value^(j->dnodes[i]->D>=0);
            }
        }
        // cout<<"after forward check node num is "<<j->dnodes[i]->num<<", value: "<<j->dnodes[i]->value<<", D: "<<j->dnodes[i]->D<<endl;
        current_Frontier.updateDfrontier(j->dnodes[i]);
        if(j->dnodes.size()>0&&j->dnodes[i]->value!=2)
        {
            imply(j->dnodes[i],j->dnodes[i]->value,current_Frontier);
        }
        // cout<<"after simulate node num is "<<j->dnodes[i]->num<<", value: "<<j->dnodes[i]->value<<endl;
        current_Frontier.updateDfrontier(j->dnodes[i]);
        if(j->dnodes.size()>0&&j->dnodes[i]->value!=2)
        {
            imply(j->dnodes[i],j->dnodes[i]->value,current_Frontier);
        }
    }
    return;
}
bool activatedFault(node_class* fault_node)
{
    if(fault_node->value!=2)
    {
        if(fault_node->D<0) return false;
        else return true;
    }
    else
    {
        return true;
    }
}
bool existsXPathToPO(node_class* fault_node)
{
    if(fault_node->dnodes.size()==0) return true;
    for(int i=0;i<fault_node->dnodes.size();i++)
    {
        if(fault_node->dnodes[i]->value==2||fault_node->dnodes[i]->D>=0) 
        {
            if(existsXPathToPO(fault_node->dnodes[i])) return true;
            // else return false;
        }
        else
        {
            return false;
        }
        // cout<<"my node num: "<<fault_node->num<<endl;
    }
    // cout<<"my node num: "<<fault_node->num<<endl;

    // If no path to a PO with an X value is found, return false
    return false;
}
bool testImpossible(node_class *d_node){
    if(!activatedFault(d_node)){
        // cout << "Fault cannot be activated." << endl;
        return false;
    }
     if (!existsXPathToPO(d_node)) {
        // cout << "No X-path to primary output exists." << endl;
        return false;
    }
    return true;
}
bool podem_sub(node_class *d_node,frontier &current_Frontier){
    node_class *npp;
    vector<node_class> temp_node;
    frontier temp_frontier;
    for(int i=0; i<Npo; i++)
    {
        if ((Poutput[i]->D>=0)&&Poutput[i]->value!=2)
        {
            return true;
        }
    }
    if (!testImpossible(d_node))
    {
     
        return false;
    }
    auto pair1 = objective(d_node,!d_node->D,current_Frontier);
    node_class *k=pair1.first;
    int v_k=pair1.second;
    auto pair2= backtrace(k, v_k);
    node_class *j=pair2.first;
    int v_j=pair2.second;
    temp_node = Node;
    temp_frontier = current_Frontier;
    imply(j, v_j,current_Frontier);
    // for(int i = 0; i < Nnodes; i++) 
    // {
    //     cout<<"after imply  num:\t"<<(&Node[i])->num<<"\tvalue:\t"<<(&Node[i])->value<<"\tD:\t"<<(&Node[i])->D<<endl;
    // }

    if (podem_sub(d_node,current_Frontier)) return true;
    // cout<<"failure, backtrack by inverting PI value"<<endl;
    Node = temp_node;
    // for(int i=0;i<Nnodes;i++)
    // {
    //     cout<<"restore the old num:\t"<<Node[i].num<<"\tvalue:\t"<<Node[i].value<<"\tD:\t"<<Node[i].D<<endl;
    // }
    current_Frontier = temp_frontier;   

    //failure, backtrack by inverting PI value
    imply(j, !v_j,current_Frontier);

    if (podem_sub(d_node,current_Frontier)) return true;
    // cout<<"failure again, resetting PI j"<<endl;
    Node = temp_node;
    // for(int i=0;i<Nnodes;i++)
    // {
    //     cout<<"restore the old num:\t"<<Node[i].num<<"\tvalue:\t"<<Node[i].value<<"\tD:\t"<<Node[i].D<<endl;
    // }   
    current_Frontier = temp_frontier;
    //failure again, resetting PI j
    // backtrack to parent state in the decision tree
    imply(j, 2,current_Frontier);


    return false;
}

void podem()
{
    unsigned target_num;
    frontier current_Frontier;
    node_class *np,*d_start_np;
    int stuck_at;
    istringstream ss(cp);
    string path;
    string result1="",result2="";
    ss >> target_num >> stuck_at >> path;
    ofstream output_file(path);
    for(int i=0;i<Nnodes;i++)
    {
        np=&Node[i];
        if(np->num==target_num)
        {
            d_start_np = np;
            np->value = 2;
            np->D = (stuck_at==1)? 0:1;
            // current_Frontier.updateDfrontier(np);
        }
        else
        {
            np->value=2;
        }
    }

    if(!podem_sub(d_start_np,current_Frontier))
    {
        // cout<<"podem failed!!"<<endl;
        for(int i=0;i<Pinput.size();i++)
        {
            np = Pinput[i];
            np->value = 2;
        }
    }
    else
    {
        // cout<<"PODEM successful"<<endl;
    }

    if(!output_file.is_open())
    {
        cerr<<"Cannot open a input file to do logicsim"<<endl;
        return;
    }

    //Write the PI ID to the first line of output file
    for(int i=0;i<Pinput.size();i++)
    {
        np = Pinput[i];
        result1 += std::to_string(np->num)+",";
    }
    result1.pop_back();
    output_file<<result1<<endl;//Write PI IDs
    bool allTwo = true;
    result2 = "";
    for(int i = 0; i < Pinput.size(); i++) {
        np = Pinput[i];
        if(np->D >= 0) np->value = !np->value;
        if(np->value != 2) allTwo = false;
        result2 += std::to_string(np->value) + ",";
    }
    result2.pop_back();
    std::replace(result2.begin(), result2.end(), '2', 'x'); //replace all 2 -> 0
    if(!allTwo) 
    {
        output_file<<result2<<endl;
    }
    output_file.close();
}

/*========================= TPG =====================================*/
vector<string> fault_order_v0 (unordered_set<string>& fault_list){
    unordered_map<unsigned, int> num_level_pair;
    for (int i=0; i<Nnodes; i++){
        unsigned node_num = Node[i].num;
        int node_level = Node[i].level;
        num_level_pair[node_num] = node_level;
    }

    vector<unsigned> vec_fault_node;
    vector<int> vec_fault_value;
    vector<int> vec_fault_level;
    for (auto& element: fault_list){
        size_t at = element.find('@');
        if (at != string::npos){
            string fault_node_str = element.substr(0, at);
            string fault_value_str = element.substr(at+1);

            unsigned fault_node = stoi(fault_node_str);
            int fault_value = stoi(fault_value_str);
            vec_fault_node.push_back(fault_node);
            vec_fault_value.push_back(fault_value);

            int fault_level = num_level_pair[fault_node];
            vec_fault_level.push_back(fault_level);
        }
    }
   
    vector<size_t> idx(vec_fault_node.size());
    for (size_t i = 0; i < idx.size(); i++) {
        idx[i] = i;
    }

    sort(idx.begin(), idx.end(), [&vec_fault_level](size_t i1, size_t i2) {
        return vec_fault_level[i1] < vec_fault_level[i2]; //sort based on vec_fault_level
    });

    //reorder vec_fault_node, vec_fault_value, and vec_fault_level based on sorted indices
    vector<unsigned> vec_fault_node_sorted(vec_fault_node.size());
    vector<int> vec_fault_value_sorted(vec_fault_value.size());
    vector<int> vec_fault_level_sorted(vec_fault_level.size());

    for (size_t i = 0; i < idx.size(); i++) {
        vec_fault_node_sorted[i] = vec_fault_node[idx[i]];
        vec_fault_value_sorted[i] = vec_fault_value[idx[i]];
        vec_fault_level_sorted[i] = vec_fault_level[idx[i]];
    }

    vector<string> vec_fault_list;
    for (size_t i = 0; i < vec_fault_node.size(); i++){
        string fault_element = to_string(vec_fault_node_sorted[i]) + "@" + to_string(vec_fault_value_sorted[i]);
        vec_fault_list.push_back(fault_element);
    }

    return vec_fault_list;
}

void fault_selection(unordered_set<string> fault_list,unordered_set<string>::iterator& it)
{
    int index;
    vector<string> fault_list_order;
    if(params["fo"]=="")
    {
        srand(time(NULL));
        index = rand()%fault_list.size();
        advance(it, index);
        cp = it->substr(0, it->find('@')) + " "+ it->substr(it->find('@') + 1) +" ../atpg/"+params["alg"]+".tp";
    }
    else if (params["fo"]=="v0")
    {
        fault_list_order = fault_order_v0(fault_list);
        cp = fault_list_order.begin()->substr(0,fault_list_order.begin()->find('@')) + " " + fault_list_order.begin()->substr(fault_list_order.begin()->find('@')+1) +" ../atpg/"+params["alg"]+".tp";
    }
    
}
void tpg()
{
    auto start = chrono::high_resolution_clock::now();
    istringstream ss(cp);
    ifstream input_file;
    ofstream output_file;
    string cmd,param,path,line,tp,result;
    unordered_set<string> fault_list,remain_fault;
    fault_coverage = 0;
    int tp_add_count = 0;
    remain_fault.clear();

    
    cp = "../fl/fl_result.txt";
    fl();
    input_file.open(cp);
    while(getline(input_file,line))
    {
        fault_list.insert(line);
    }
    input_file.close();

    while(ss>>cmd)
    {
        if(cmd == "-rtp")
        {
            ss>>param;
            params["rtp"] = param;
            if(params["rtp"]=="v0")
            {
                ss>>param;
                params["rtp_v0_fc"] = param;
            }
            else if(params["rtp"]=="v1")
            {
                ss>>param;
                params["rtp_v1_fc"] = param;
            }
            else if(params["rtp"]=="v2")
            {
                ss>>param;
                params["rtp_v2_fc"] = param;
            }
        }
        else if (cmd == "-df")
        {
            ss>>param;
            params["df"] = param;
            cout<<"df has par:"<<param<<endl;
            if(params["df"]=="cc")
            {
                scoap();
            }
            if(params["df"]=="ll"||params["df"]=="lh"){
                lev_2();
            }
        }
        else if (cmd == "-jf")
        {
            ss>>param;
            params["jf"] = param;
            cout<<"jf has par: "<<param<<endl;
            if(params["jf"]=="v0")
            {
                scoap();
            }
        }
        else if(cmd == "-fl")
        {
            ss>>param;
            params["fl"] = param;
        }
        else if (cmd == "-fo")
        {
            ss>>param;
            params["fo"] = param;
            lev_2();
        }
        else if (cmd =="DALG" || cmd =="PODEM")
        {
            params["alg"] = cmd;
        }
        else
        {
            path = cmd;
        }
    }
    output_file.open(path);
    //sort PI_ID
    vector<node_class*> sorted_Pins(Pinput.begin(), Pinput.end());
    std::sort(sorted_Pins.begin(), sorted_Pins.end(), sort_num_input);
    for(int z=0;z<Npi;z++)
    {
        result += std::to_string(sorted_Pins[z]->num)+",";
    }
    result.pop_back(); //erase the last comma
    output_file<<result;
    output_file.close();
    if (params["rtp"]=="v0")
    {
        while((stof(params["rtp_v0_fc"])>fault_coverage))
        {
            auto it = fault_list.begin();
            fault_selection(fault_list,it);
            // cp = it->substr(0, it->find('@')) + " "+ it->substr(it->find('@') + 1) +" ../atpg/"+params["alg"]+".tp";
            cout<<cp<<endl;
            if(params["alg"]=="DALG")
            {
                dalg();
            }
            else
            {
                podem();
            }
            cout<<"DALG finished!"<<endl;
            circuits_value_reset();
            input_file.open("../atpg/"+params["alg"]+".tp");
            getline(input_file,line);
            getline(input_file,line);
            tp = line;
            input_file.close();
            cout<<"tp: "<<tp<<endl;
            if(tp.empty())
            {
                remain_fault.insert(*it);
                // cout<<"the remain fault is :"<<*it<<endl;
                fault_list.erase(*it);
                continue;
            }
            else
            {
                std::replace(tp.begin(), tp.end(), 'x', '0'); //replace all x -> 0
            }
            output_file.open(path,ios::app);
            output_file<<endl<<tp;
            tp_add_count++;
            output_file.close();
            cp = path +" "+ to_string(tp_add_count)+ " " + "../fc/tpfc_result.txt";
            tpfc();
            for( int i=0;i<unique_faults_for_pattern.size();i++)
            {
                fault_list.erase(to_string(unique_faults_for_pattern[i].first)+"@"+to_string(unique_faults_for_pattern[i].second));
            }
            cout<<"The fault coverage is: "<<fault_coverage<<endl;
            cout<<"the fault still need to be find: ";
            for(auto it = fault_list.begin();it!=fault_list.end();++it)
            {
                cout<<*it<<" ";
            }
            cout<<endl;
        }
        for(auto it = fault_list.begin();it!=fault_list.end();it++)
        {
            remain_fault.insert(*it);
        }        
        cout<<"The fault coverage is: "<<fault_coverage<<endl;
        cout<<"the fault remain is:";
        for(auto it = remain_fault.begin();it!=remain_fault.end();it++)
        {
            cout<<*it<<" ";
        }
        cout<<endl;
    }
    else if(params["rtp"]=="v1")
    {
        while(stof(params["rtp_v1_fc"])>fault_coverage)
        {
            cp="1000 b ../tp/rtpg.tp";
            rtpg();
            circuits_value_reset();
            input_file.open("../tp/rtpg.tp");
            getline(input_file,line);
            getline(input_file,line);
            tp = line;
            input_file.close();
            tp_add_count++;
            cp = "../tp/rtpg.tp "+ to_string(tp_add_count)+ " " + "../fc/tpfc_result.txt";
            // cout<<cp<<endl;
            tpfc();
            for( int i=0;i<unique_faults_for_pattern.size();i++)
            {
                fault_list.erase(to_string(unique_faults_for_pattern[i].first)+"@"+to_string(unique_faults_for_pattern[i].second));
            }
            // cout<<"The fault coverage is: "<<fault_coverage<<endl;
            // cout<<"the fault remain is:";
            // for(auto it = fault_list.begin();it!=fault_list.end();it++)
            // {
            //     cout<<*it<<" ";
            // }
            // cout<<endl;
        }
        for(auto it = fault_list.begin();it!=fault_list.end();it++)
        {
            remain_fault.insert(*it);
        }        
        cout<<"The fault coverage is: "<<fault_coverage<<endl;
        cout<<"the fault remain is:";
        for(auto it = remain_fault.begin();it!=remain_fault.end();it++)
        {
            cout<<*it<<" ";
        }
        cout<<endl;
        rtpg_call_count=0;
    }
    else if(params["rtp"]=="v2")
    {
        float delta_fault_coverage = 100,pre_fault_coverage;
        while(stof(params["rtp_v2_fc"])<delta_fault_coverage)
        {
            pre_fault_coverage = fault_coverage;
            auto it = fault_list.begin();
            fault_selection(fault_list,it);
            // cp = it->substr(0, it->find('@')) + " "+ it->substr(it->find('@') + 1) +" ../atpg/"+params["alg"]+".tp";
            // cout<<cp<<endl;
            if(params["alg"]=="DALG")
            {
                dalg();
            }
            else
            {
                podem();
            }
            circuits_value_reset();
            input_file.open("../atpg/"+params["alg"]+".tp");
            getline(input_file,line);
            getline(input_file,line);
            tp = line;
            input_file.close();
            // cout<<"tp: "<<tp<<endl;
            if(tp.empty())
            {
                remain_fault.insert(*it);
                // cout<<"the remain fault is :"<<*it<<endl;
                fault_list.erase(*it);
                continue;
            }
            else
            {
                std::replace(tp.begin(), tp.end(), 'x', '0'); //replace all x -> 0
            }
            output_file.open(path,ios::app);
            output_file<<endl<<tp;
            tp_add_count++;
            output_file.close();
            cp = path +" "+ to_string(tp_add_count)+ " " + "../fc/tpfc_result.txt";
            tpfc();
            for( int i=0;i<unique_faults_for_pattern.size();i++)
            {
                fault_list.erase(to_string(unique_faults_for_pattern[i].first)+"@"+to_string(unique_faults_for_pattern[i].second));
            }
            // cout<<"The fault coverage is: "<<fault_coverage<<endl;
            // cout<<"the fault still need to be find: ";
            // for(auto it = fault_list.begin();it!=fault_list.end();++it)
            // {
            //     cout<<*it<<" ";
            // }
            // cout<<endl;
            delta_fault_coverage = fault_coverage-pre_fault_coverage;
            cout<<"delta fault coverage is "<<delta_fault_coverage<<endl;
            if(fault_list.empty())
            {
                break;
            }
        }
        for(auto it = fault_list.begin();it!=fault_list.end();it++)
        {
            remain_fault.insert(*it);
        }        
        cout<<"The fault coverage is: "<<fault_coverage<<endl;
        cout<<"the fault remain is:";
        for(auto it = remain_fault.begin();it!=remain_fault.end();it++)
        {
            cout<<*it<<" ";
        }
        cout<<endl;
    }
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << "Execution time: " << duration.count() << " ms" << endl;
    
}


/*========================= End of program ============================*/