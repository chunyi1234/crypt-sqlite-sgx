#include <iostream>
#include <string>
#include <stdlib.h>
#include "sgx_urts.h"
#include "Enclave_u.h" // Headers for untrusted part (autogenerated by edger8r)
#include "sgx_tcrypto.h"
#include "aes-string.h"
#include <string.h>
#include <stdint.h>
#include <openssl/aes.h>
#include <openssl/des.h>
#include <openssl/rand.h> 
#include <openssl/hmac.h>
#include <openssl/buffer.h>
#include<openssl/sha.h>
#include<sqlite3.h>
#include<assert.h>
#include"rocksdb/db.h"
#include<sstream>
//using namespace std;
using namespace std;
// #define XDSGX_BLOCK_SIZE  AES_BLOCK_SIZE
#define XDSGX_BLOCK_SIZE  65535
#define AES_KEY_SIZE 16
#define MEM_BLOCK_SIZE 1024*8   //1k
# define MAX_PATH FILENAME_MAX
# define ENCLAVE_FILENAME "enclave.signed.so"
sgx_enclave_id_t eid = 0;


int aes_encrypt(char *key_string,const char *sql,unsigned char *out1){
    AES_KEY  aes;
    int sql_len = strlen(sql);
   // printf("sql_len = %d\n", sql_len);
    int n=(sql_len-1)/16+1;
    if (AES_set_encrypt_key((unsigned char*)key_string, 128, &aes) < 0) {
        fprintf(stderr, "Unable to set encryption key in AES\n");
        return 0;
    }
    unsigned char* tmpe=(unsigned char*)malloc(sizeof(unsigned char)*(16*n+1));
    memcpy(tmpe,sql,sql_len);
    for(int m=sql_len;m<(n*16+1);m++){
        tmpe[m]='\0';
    }
    for(int m=0;m<n;m++){
        AES_encrypt(tmpe+16*m,out1+16*m,&aes);
    }
    return 1;
}


int aes_decrypt(char *key_string, unsigned char *out1, unsigned char *out2){
    AES_KEY  aes;
    int sql_len = strlen((char *)out1);
    //printf("sql_len = %d\n", sql_len);
    int n = (sql_len-1)/16+1;
    if (AES_set_decrypt_key((unsigned char*)key_string, 128, &aes) < 0) {
        fprintf(stderr, "Unable to set encryption key in AES\n");
        return 0;
    }
    for(int m=0;m<n;m++){
        AES_decrypt(out1+16*m,out2+16*m,&aes);

    }
    
        
    
    return 1;
}


struct ctr_state 
{ 
    unsigned char ivec[AES_BLOCK_SIZE];  
    unsigned int num; 
    unsigned char ecount[AES_BLOCK_SIZE]; 
}; 
AES_KEY key, dec_key; 

int bytes_read, bytes_written;   
unsigned char indata[XDSGX_BLOCK_SIZE]; 
unsigned char outdata[XDSGX_BLOCK_SIZE];
unsigned char iv[AES_BLOCK_SIZE]; //16?
struct ctr_state state;

int init_ctr(struct ctr_state *state, const unsigned char iv[16])
{        
    /*O aes_ctr128_encrypt exige um 'num' e um 'ecount' definidos a zero na primeira chamada. */
    state->num = 0;
    //memset(state->ecount, 0, XDSGX_BLOCK_SIZE); //16?

    /* Inicilaização do contador no 'ivec' a 0 */
    memset(state->ecount, 0, 16); //16?
    // memset(state->ivec + 8, 0, 8);
    /* Copia o IV para o 'ivec' */
    memcpy(state->ivec, iv, 16); //16?
}

unsigned char * TextEncrypt(const unsigned char* enc_key, const unsigned char * text, int bytes_read)
{ 
    //Cria vector com valores aleatórios
    if(!RAND_bytes(iv, AES_BLOCK_SIZE))
    {
        printf("Erro\n");
        exit(1);    
    }

    //printf("enc_key = %s\n",enc_key);
    //Inicializa a chave de encriptação
    if (AES_set_encrypt_key(enc_key, 128, &key) < 0)
    {
        fprintf(stderr, "Unable to set encryption key in AES");
        exit(1);
    }
/*    if (AES_set_decrypt_key(enc_key, 128, &dec_key) < 0)
    {
        fprintf(stderr, "Unable to set encryption key in AES");
        exit(1);
    }*/

    init_ctr(&state, iv); //Chamada do contador

    // printf("state.ivec: ");
    // for(int i=0; i<AES_BLOCK_SIZE; i++){
    //     printf("%02x ", state.ivec[i]);
    // }
    // printf("\n");
    // printf("state.ecount: ");
    // for(int i=0; i<AES_BLOCK_SIZE; i++){
    //     printf("%02x ", state.ecount[i]);
    // }
    // printf("\n");
    // printf("state.num:%d\n",state.num);

    // //AES_set_encrypt_key(enc_key, 128, &key);    
    // printf("state.num:%d\n",state.num);
    //Encripta em blocos de 16 bytes e guarda o texto cifrado numa string -> outdata
    AES_ecb_encrypt(text,outdata,&key,1);
    //AES_cbc128_encrypt(text, outdata, bytes_read, &key, state.ivec, state.ecount, &state.num);
    printf("soutdata: ");
    for(int i=0; i<bytes_read; i++){
        printf("%02x ", outdata[i]);
    }
    printf("\n");
    memcpy(state.ivec, iv, 16);

    fflush(stdin);
    return outdata;
}

unsigned char * TextDecrypt(const unsigned char* enc_key, unsigned char* cypherText,int bytes_read)
{       

    //Inicialização da Chave de encriptação 
    // if (AES_set_encrypt_key(enc_key, 128, &key) < 0)
    // {
    //     fprintf(stderr, "Unable to set decryption key in AES.");
    //     exit(1);
    // }

    //init_ctr(&state, ivec);//Chamada do contador
    // printf("TextDecrypt state.num=%d,ivec=%d,ecount=%d\n",state.num,state.ivec,state.ecount);
    //memcpy(state.ivec, iv, 16);
    //Encripta em blocos de 16 bytes e escreve o ficheiro output.txt cifrado         
    //bytes_read = strlen(cypherText);    
    if (AES_set_decrypt_key(enc_key, 128, &dec_key) < 0)
    {
        fprintf(stderr, "Unable to set encryption key in AES");
        exit(1);
    }
    // state.ivec = ecount;
    //memcpy(state.ivec, ecount, 16);
    //AES_ecb_encrypt(cypherText,outdata,&key,0);
    //AES_ctr128_encrypt(cypherText, outdata, bytes_read, &key, state.ivec, state.ecount, &state.num);
    // printf("decrypt data in TextDecrypt: ");
    // for(int i=0; i<bytes_read; i++){
    //     printf("%c", outdata[i]);
    // }
    // printf("\n");
    fflush(stdin);
    return outdata;
}
void encryppp(const char* str){


}
// ocalls for printing string (C++ ocalls)
void ocall_print_error(const char *str){
    cerr << str << endl;
}

void ocall_print_string(const char *str){
    cout << str;
}

void ocall_println_string(const char *str){
    cout << str << endl;
}
uint8_t key1[4] = {0xef,0x45,0x3f,0xa6};
// Application entry
void stream2hex(const std::string str, std::string& hexstr, bool capital = false)
{
    hexstr.resize(str.size() * 2);
    const size_t a = capital ? 'A' - 1 : 'a' - 1;

    for (size_t i = 0, c = str[0] & 0xFF; i < hexstr.size(); c = str[i / 2] & 0xFF)
    {
        hexstr[i++] = c > 0x9F ? (c / 16 - 9) | a : c / 16 | '0';
        hexstr[i++] = (c & 0xF) > 9 ? (c % 16 - 9) | a : c % 16 | '0';
    }
}

// Convert string of hex numbers to its equivalent char-stream
void hex2stream(const std::string hexstr, std::string& str)
{
    str.resize((hexstr.size() + 1) / 2);

    for (size_t i = 0, j = 0; i < str.size(); i++, j++)
    {
        str[i] = (hexstr[j] & '@' ? hexstr[j] + 9 : hexstr[j]) << 4, j++;
        str[i] |= (hexstr[j] & '@' ? hexstr[j] + 9 : hexstr[j]) & 0xF;
    }
}

int updatequery(void){
       //uint8_t* sql =(uint8_t*) input.c_str();
       //uint8_t* stringsql= (uint8_t*)calloc(1,500); 
       //uint8_t* encsql=(uint8_t*)calloc(1,500); 
    //encrypp(sql,encsql,key);
    //string encc;
    //encc=(char*)encsql;
    //cout<<encc;
    return 0;
}

string insertquery(void){
    string input;
     //uint8_t* sql =(uint8_t*) input.c_str();
       //uint8_t* stringsql= (uint8_t*)calloc(1,500); 
       //uint8_t* encsql=(uint8_t*)calloc(1,500); 
       string table;
       string colum1;
       string colum2;
       string value1;
       string value2;
        cout<<"insert into "; 
       cin>>table;
       cin.get();
       std::cout.flush();
       cout<<"insert into "<<table<<"(";
       
       cin>>colum1;
       cin.get();
        std::cout.flush();
        cout<<"insert into "<<table<<"("<<colum1<<",";
       cin>>colum2;
       cin.get();
        std::cout.flush();
     cout<<"insert into "<<table<<"("<<colum1<<","<<colum2<< ") values (";
      cin>>value1;
      cout<<value1;
      cin.get();
       //std::cout.flush();
      cout<<"insert into "<<table<<"("<<colum1<<","<<colum2<< ") values ("<<value1<<",";
     cin>> value2;
     cin.get();
    //std::cout.flush();
     cout<<"insert into "<<table<<"("<<colum1<<","<<colum2<< ") values ("<<value1<<","<<value2<<")";
       cout<<"\n";

    //define text and enctext   
    int inputlen;
    char* dtable=(char*)table.c_str();
    inputlen=strlen(table.c_str());
    unsigned char etable[17];
    //uint8_t* etable=(uint8_t*)calloc(1,500); 
    char* dcolum1=(char*)colum1.c_str();
    inputlen=strlen(colum1.c_str());
    unsigned char ecolum1[17];
    //uint8_t* ecolum1=(uint8_t*)calloc(1,500); 
    char* dcolum2=(char*)colum2.c_str();
    inputlen=strlen(colum2.c_str());
    unsigned char ecolum2[17];
    //uint8_t* ecolum2=(uint8_t*)calloc(1,500); 
    char* dvalue1=(char*)value1.c_str();
    inputlen=strlen(value1.c_str());
    unsigned char evalue1[17];
    //uint8_t* evalue1=(uint8_t*)calloc(1,500); 
    char* dvalue2=(char*)value2.c_str();
    inputlen=strlen(value2.c_str());
    unsigned char evalue2[17];
    //uint8_t* evalue2=(uint8_t*)calloc(1,500); 


    //encrypt key
    char* key_aes="1234567890";


    aes_encrypt(key_aes,dtable,etable);
    etable[16]='\0';
    aes_encrypt(key_aes,dcolum1,ecolum1);
    ecolum1[16]='\0';
    aes_encrypt(key_aes,dcolum2,ecolum2);
    ecolum2[16]='\0';
    aes_encrypt(key_aes,dvalue1,evalue1);
    evalue1[16]='\0';
    aes_encrypt(key_aes,dvalue2,evalue2);
    evalue2[16]='\0';

    //convert to string
    table=(char*)etable;
    colum1=(char*)ecolum1;
    colum2=(char*)ecolum2;
    value1=(char*)evalue1;
    value2=(char*)evalue2;


//a try for hex
stream2hex(table,table);
stream2hex(colum1,colum1);
stream2hex(colum2,colum2);
stream2hex(value1,value1);
stream2hex(value2,value2);

    //construct the ensql 
    string ensql;
    ensql="insert into a";
    ensql+=table;
    cout<<ensql<<endl;
    ensql+="(a";
    ensql+=colum1;
    cout<<ensql<<endl;
    ensql+=",a";
    ensql+=colum2;
    cout<<ensql<<endl;
    ensql+=") values(\'";
    ensql+=value1;
    cout<<ensql<<endl;
    ensql+="\',\'";
    ensql+=value2;
    cout<<ensql<<endl;
    ensql+="\')";
    cout<<ensql<<endl;

    return ensql;
       

}
int selectrange(float left,float right,string table,string column){
    
    //   sgx_enclave_id_t eid=0;
   // ecall_select_bothends(eid,left,right);
    return 0;
}
int selectquery(void){
        string table;
       string colum1;
       string colum2;
       char* key_aes="1234567890";
       cout<<"select ";
       string input;
       string query;
       
       cin>>input;
       if(input=="*"){
           cout<<"select * from ";
           cin>>table;
           cin.get();
           char* dtable=(char*)table.c_str();
           unsigned char etable[17];
           aes_encrypt(key_aes,dtable,etable);
           etable[16]='\0';
           table=(char*)etable;
           stream2hex(table,table);
           cout<<"select * from a"<<table<<"where ";
           string allrange;
           cin>>allrange;
           cin.get();
           if(allrange=="all"){
                query="select * from a"+table;
                cout<<query;
                cout<<endl;
                ecall_execute_sql(eid,query.c_str());
           }
           else{
               cout<<"select * from a"<<table<<" where ";
                float left;
                float right;
                cin>>left;
                cin.get();
                cout<<"select * from a"<<table<<" where "<<left<<"<";
                string rangecolumn;
                cin>>rangecolumn;
                cin.get();
                cout<<"select * from a"<<table<<" where "<<left<<"<"<<rangecolumn<<"<";
                cin>>right;
                cin.get();
                selectrange(left,right,table,rangecolumn);
           }
            
           

       }
       
    return 0;
}


int aesdec(unsigned char* enc,unsigned char* src){

    return 0;
}
int aesenc(unsigned char* src,unsigned char* enc){


    return 0;
}

int createcreate(string table,string column1,string colum2){
    return 0;
}
int insertinsert(string table,string column1,string column2,float value1,float value2){
    return 0;

}
int selectselect(string table,string column1,string column2,float min,float max){
    return 0;
}
int dropquery(void){
    return 0;
}

string createquery(void){
     string table;
       string colum1;
       string colum2;
        cout<<"create table "; 
       cin>>table;
       cin.get();
       std::cout.flush();
       
       cout<<"create table "<<table<<"(";
       
       cin>>colum1;
       cin.get();
        std::cout.flush();
        cout<<"create table "<<table<<"("<<colum1<<" string,";
       cin>>colum2;
       cin.get();
         cout<<"create table "<<table<<"("<<colum1<<" string,"<<colum2<<" string)";
    int inputlen;
    char* dtable=(char*)table.c_str();
    inputlen=strlen(table.c_str());
    unsigned char etable[17];
    //uint8_t* etable=(uint8_t*)calloc(1,500); 
    char* dcolum1=(char*)colum1.c_str();
    inputlen=strlen(colum1.c_str());
    unsigned char ecolum1[17];
    //uint8_t* ecolum1=(uint8_t*)calloc(1,500); 
    char* dcolum2=(char*)colum2.c_str();
    inputlen=strlen(colum2.c_str());
    unsigned char ecolum2[17];

    char* key_aes="1234567890";


    aes_encrypt(key_aes,dtable,etable);
    etable[16]='\0';
    aes_encrypt(key_aes,dcolum1,ecolum1);
    ecolum1[16]='\0';
    aes_encrypt(key_aes,dcolum2,ecolum2);
    ecolum2[16]='\0';

    table=(char*)etable;
    colum1=(char*)ecolum1;
    colum2=(char*)ecolum2;

    stream2hex(table,table);
stream2hex(colum1,colum1);
stream2hex(colum2,colum2);
//stream2hex(value1,value1);
//stream2hex(value2,value2);

    //construct the ensql 
    string ensql;
    ensql="create table a";
    ensql+=table;
    cout<<ensql<<endl;
    ensql+="(a";
    ensql+=colum1;
    cout<<ensql<<endl;
    ensql+=" string,a";
    ensql+=colum2;
    cout<<ensql<<endl;
    ensql+=" string)";
    cout<<ensql;

    return ensql;
    //return "\0";
}

int main10(){
    //this main is for test;

}
int virtualsgx(string value1){
    char* key_aess="1234567890";
    string input=value1;
    int inputlen=strlen(input.c_str());
    
    unsigned char out11[17];
    aes_encrypt(key_aess,input.c_str(),out11);
    out11[16] = '\0';
    unsigned char out22[17];
    aes_decrypt(key_aess,out11,out22);
    string output=(char*)out11;
    
    return 0;

};
string decstring(string value1);
void virtualtime(void);
string virtualsgxmin(string value1,float min){
    virtualtime();
    string decvalue1=decstring(value1);
    stringstream ss;
    ss<<decvalue1;
    double test;
    ss>>test;
    if(test>min){
        return value1;
    }
    return "null";
}
string virtualsgxmax(string value1,float max){
    virtualtime();
    string decvalue1=decstring(value1);
    stringstream ss;
    ss<<decvalue1;
    double test;
    ss>>test;
    if(test<max){
        return decvalue1;
    }
    return "null";
}
void virtualtime(void){
    int i=0;
    while(1){
        i++;
        if(i>100)
            break;
    }
    return;
}
string encstring(string value1){
    char* key_aess="1234567812345678s";
    string input=value1;
    int inputlen=strlen(input.c_str());
    int n=(inputlen-1)/16+1;
    unsigned char *out11=(unsigned char*)malloc(sizeof(char)*16*n+1);
    aes_encrypt(key_aess,input.c_str(),out11);
    out11[16*n] = '\0';
    string output=(char*)out11;
    stream2hex(output,output);
    return output;
}

string decstring(string value1){
    char* key_aess="1234567812345678";
    string input=value1;
    hex2stream(input,input);
    int inputlen=strlen(input.c_str());
    int n=(inputlen-1)/16+1;
    unsigned char *out11=(unsigned char*)malloc(sizeof(char)*16*n+1);
    aes_decrypt(key_aess,(unsigned char*)input.c_str(),out11);
    out11[16*n]='\0';
    string output=(char*)out11;
    //hex2stream(output,output);
    return output;
}

string hash1(string value1){
    //use sha256
    string value=value1+"111";
    const unsigned char* input= (const unsigned char* )value.c_str();
    size_t length=value.length();

    unsigned char md[33];
    SHA256(input,length,md);
    string value2;
    md[32]='\0';
    value2=(char*)md;
    stream2hex(value2,value2);
    return value2;
}
string hash2(string value1){
    string value=value1+"222";
    const unsigned char* input= (const unsigned char* )value.c_str();
    size_t length=value.length();
    unsigned char md[33];
    SHA256(input,length,md);
    string value2;
    md[32]='\0';
    value2=(char*)md;
    stream2hex(value2,value2);
    return value2;
}
string peseudo_permutation_P(string key111,string value1){
    char* key_aess=(char*)key111.c_str();
    string input=value1;
    int inputlen=strlen(input.c_str());
    int n=(inputlen-1)/16+1;
    unsigned char *out11=(unsigned char*)malloc(sizeof(char)*16*n+1);
    aes_encrypt(key_aess,input.c_str(),out11);
    out11[16*n] = '\0';
    string output=(char*)out11;
    stream2hex(output,output);
    return output;
}
string peseudo_inverse_permutation_P(string key111,string value1){
    char* key_aess=(char*)key111.c_str();
    //cout<<key111<<endl;
    //printf("%s\n",key_aess);
    string input=value1;
    hex2stream(input,input);
    int inputlen=strlen(input.c_str());
    int n=(inputlen-1)/16+1;
    unsigned char *out11=(unsigned char*)malloc(sizeof(char)*16*n+1);
    aes_decrypt(key_aess,(unsigned char*)input.c_str(),out11);
    out11[16*n]='\0';
    string output=(char*)out11;
    //hex2stream(output,output);
    return output;
}
string pseudo_F(string key111,string value1){
    char* key_aess=(char*)key111.c_str();
    string input=value1;
    int inputlen=strlen(input.c_str());
    int n=(inputlen-1)/16+1;
    unsigned char *out11=(unsigned char*)malloc(sizeof(char)*16*n+1);
    aes_encrypt(key_aess,input.c_str(),out11);
    out11[16*n] = '\0';
    string output=(char*)out11;
    stream2hex(output,output);
    return output;
}
string pseudo_inverse_F(string key111,string value1){
    char* key_aess=(char*)key111.c_str();
    //cout<<key111<<endl;
    //printf("%s\n",key_aess);
    string input=value1;
    hex2stream(input,input);
    int inputlen=strlen(input.c_str());
    int n=(inputlen-1)/16+1;
    unsigned char *out11=(unsigned char*)malloc(sizeof(char)*16*n+1);
    aes_decrypt(key_aess,(unsigned char*)input.c_str(),out11);
    out11[16*n]='\0';
    string output=(char*)out11;
    //hex2stream(output,output);
    return output;
}
int main(){
    //keep in mind that strlen of key must be 16n ,or not you will face unbelieveable questions;
    string value1="jsjsjsjsjsdsfsjssjjjjjjjjjjjjjjj";
    cout<<value1.length()<<":   "<<encstring(value1).length()<<":   "<<encstring(value1)<<endl;
    cout<<decstring(encstring(value1))<<endl;
    //virtualsgx(1.654,value1);
    string value2="1.78234";
    //float(value2)
    double test;
    std::stringstream ss;
    ss<<value2;
    ss>>test;
    cout<<test<<endl<<endl;
    cout<<virtualsgxmax(encstring(value2),1)<<endl;
    cout<<"test for hash"<<endl;
    cout<<"length:"<<hash1(value1).length()<<endl<<"value:"<<hash1(value1)<<endl;
    cout<<"length:"<<hash2(value1).length()<<endl<<"value:"<<hash2(value1)<<endl;
    cout<<"test for pseu:"<<endl;

    cout<<value1<<endl;
    string keytest="1234567812345678";
    cout<<pseudo_F(keytest,value1)<<endl;
    cout<<pseudo_inverse_F(keytest,pseudo_F(keytest,value1))<<endl;

    return 0;
}
int main1111(int argc, char *argv[]){
    if ( argc != 2 ){
        cout << "Usage: " << argv[0] << " <database>" << endl;
        return -1;
    }
    const char* dbname = argv[1];
    
    
    char token_path[MAX_PATH] = {'\0'};
    sgx_launch_token_t token = {0};
    sgx_status_t ret = SGX_ERROR_UNEXPECTED; // status flag for enclave calls
    int updated = 0;

//cout<<encry(enc);

    //uint8_t enc[]="dsjifo";
	 ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, &token, &updated, &eid, NULL);
	//uint8_t* dec = (uint8_t*)calloc(1,500); 
	//encrypp(enc,dec, key);
    //printf("here is a test:\n the encryption returns: %s\n",dec);
    //uint8_t* decc = (uint8_t*)calloc(1,500); 
    //decrypp(dec,decc, key);
    //printf("here is a test:\n the decryption returns: %s\n",decc);
    //free(dec);
    //free(decc);

    // Initialize the enclave
   
    printf("sgx_create_enclave\n");
    if (ret != SGX_SUCCESS) {
        cerr << "Error: creating enclave" << endl;
        return -1;
    }
    cout << "Info: SQLite SGX enclave successfully created." << endl;

    

    // Open SQLite database
    ret = ecall_opendb(eid, dbname);
    if (ret != SGX_SUCCESS) {
        cerr << "Error: Making an ecall_open()" << endl;
        return -1;
    }

    cout << "Enter SQL statement to execute or 'quit' to exit: " << endl;
    string input;
     printf("hello");
    cout << "> ";
string value1="jsodjfisdf";
    virtualsgx(value1);

    printf("test for enclave\n");
    //const char testforselect[2][];

    const char* rightforselect="jisjdfio";
    const char* leftforselect="jiosdjiosjdfio";
    ecall_select_bothends(eid,leftforselect,rightforselect);
    printf("__________________________\n> ");
    while(getline(cin, input)) {
        if (input == "quit"){
            break;
        }
       /*
       uint8_t* sql =(uint8_t*) input.c_str();
       //uint8_t* stringsql= (uint8_t*)calloc(1,500); 
       uint8_t* encsql=(uint8_t*)calloc(1,500); 
       encrypp(sql,encsql,key);
       printf("encrypted string: %s",encsql);
       */
       if (input == "update"){
           cout<<'>';
           //getline(cin, input);
           updatequery();
           cout<<endl;
        }
        else if(input=="create"){
            cout<<'>';
            string ensqll;
            ensqll=createquery();
            //sqlite3_exec(dbname,ensqll)
            ret=ecall_execute_sql(eid, ensqll.c_str());
            if (ret != SGX_SUCCESS) {
            cerr << "Error: Making an ecall_execute_sql()" << endl;
            return -1;
            }
           cout<<endl;
        }
        else if (input == "insert"){
           cout<<'>';
          // getline(cin, input);
          string ensqll;
           ensqll=insertquery();
           ret =  ecall_execute_sql(eid, ensqll.c_str());
           if (ret != SGX_SUCCESS) {
            cerr << "Error: Making an ecall_execute_sql()" << endl;
            return -1;
            }
           cout<<endl;
        }
        else if(input=="select"){
            cout<<'>';
            selectquery();
            cout<<endl;
        }
        else if(input=="drop"){
            cout<<'>';
            dropquery();
            cout<<endl;
        }
        else{
                

            char* key_aes="1234567890";
            const unsigned char enc_key[17]="1234567812345678";
            //enc_key[16]='\0';
            int inputlen=strlen(input.c_str());
            unsigned char out1[((inputlen)/16+1)*16+1];
             unsigned char* ciphersql;
            ciphersql=TextEncrypt(enc_key,(unsigned char*)input.c_str(),input.length());
            aes_encrypt(key_aes,input.c_str(),out1);
            //int enclength=strlen(ciphersql);
             unsigned char* unciphersql=TextDecrypt(enc_key,ciphersql,16);
            out1[((inputlen)/16+1)*16] = '\0';
            //out1[((inputlen-1)/16+1)*16+1]='\0';
            printf("> %s\n",out1);
            printf("> %s\n",ciphersql);
            printf(">decrypt %s\n",unciphersql);
            unsigned char out2[(int)(inputlen/16+1)*16];
            aes_decrypt(key_aes,out1,out2);
            //out2[((inputlen-1)/16+1)*16+1]='\0';
            printf(">%s\n",out2);
        }
        //ret =  ecall_execute_sql(eid, sql);
       /**
        * sqlite execute
        * if (ret != SGX_SUCCESS) {
            cerr << "Error: Making an ecall_execute_sql()" << endl;
            return -1;
        }
        * 
        */
        cout << "> ";
    }

    // Closing SQLite database inside enclave
    ret =  ecall_closedb(eid);
    if (ret != SGX_SUCCESS) {
        cerr << "Error: Making an ecall_closedb()" << endl;
        return -1;
    }

    // Destroy the enclave
    sgx_destroy_enclave(eid);
    if (ret != SGX_SUCCESS) {
        cerr << "Error: destroying enclave" << endl;
        return -1;
    }

    cout << "Info: SQLite SGX enclave successfully returned." << endl;
    return 0;
}



int main1(){
    //testencypt();
    testall();
    return 0;
}

 int testencypt(void){
     string string0="hello world";
      printf("string0 is : ");
    cout<<string0<<endl;
    //int iter;
     //stream2hex(string0,string0);
    printf("string0 is : ");
    cout<<string0<<endl;
     uint8_t* string1=(uint8_t*)calloc(1,500); 
     printf("convert string0 to hex and uint8 string1:%s",string1);
     string1=(uint8_t*)string0.c_str();
     printf("convert string0 to hex and uint8 string1:%s\n",string1);
     printf("string1 has %d rang\n",find_iterations(string1));
     uint8_t* string2=(uint8_t*)calloc(1,500); 
     encrypp(string1,string2,key1);
     printf("string2(crypt string1):%s\n",string2);
     printf("string2 has %d rang\n",find_iterations(string2));
     string string3=(char*)string2;
     printf("string3(equal to string2):");
     cout<<string3<<endl;
     stream2hex(string3,string3);
     printf("convert string3 to hex:");
     cout<<string3<<endl;
     printf("string4 is a hex num:4efc5fff9a811249520cf0b94d8fa12c8585986bfb023a04d92e795f7d7c6f0b\n");
     string string4="4efc5fff9a811249520cf0b94d8fa12c8585986bfb023a04d92e795f7d7c6f0b";
     hex2stream(string4,string4);
     printf("convert string4 to stream\n");
     cout<<string4<<endl;
     uint8_t* string5=(uint8_t*)string4.c_str();
     printf("string5 is string4 to uint8\n");
     printf("string5 is %s\n",string5);
     uint8_t* string6=(uint8_t*)calloc(1,500); 
     decrypp(string5,string6,key1);
     printf("string6 is string5 decrypp:%s\n",string6);
     return 0;
 }
