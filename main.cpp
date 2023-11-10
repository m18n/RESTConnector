#include<iostream>
#include"connector_manager.h"
int main(){
    std::cout<<"START EXE\n";
    connector::connector_manager conn;
    conn.on();
    return 0;
}