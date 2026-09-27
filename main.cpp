// CodeNova Careers - minimal C++ backend
// Requires: C++17, libcurl, and nlohmann/json.hpp.
// This server accepts the application form and sends it via SMTP.
// IMPORTANT: never put your Gmail password in source code. Use a Gmail App Password
// (with 2-Step Verification enabled) or another SMTP provider credential in env vars.
#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <httplib.h>
#include <nlohmann/json.hpp>
using json=nlohmann::json;

static std::string env(const char* k,const std::string& d=""){const char* v=std::getenv(k);return v?v:d;}
static std::string esc(const std::string&s){std::string o;for(char c:s){if(c=='\\'||c=='"')o+='\\';o+=c;}return o;}

bool send_mail(const json& d,std::string& err){
    std::string smtp=env("SMTP_URL","smtps://smtp.gmail.com:465");
    std::string user=env("SMTP_USER");
    std::string pass=env("SMTP_APP_PASSWORD");
    std::string to=env("APPLICATION_TO","farhanp4567@gmail.com");
    if(user.empty()||pass.empty()){err="SMTP_USER and SMTP_APP_PASSWORD are not configured";return false;}
    std::string body="New Computer Science job application\\r\\n\\r\\n";
    body+="Name: "+d.value("name","")+"\\r\\n";
    body+="Email: "+d.value("email","")+"\\r\\n";
    body+="Phone: "+d.value("phone","")+"\\r\\n";
    body+="Position: "+d.value("position","")+"\\r\\n";
    body+="Education: "+d.value("education","")+"\\r\\n";
    body+="Experience: "+d.value("experience","")+"\\r\\n";
    body+="Skills: "+d.value("skills","")+"\\r\\n";
    body+="Resume: "+d.value("resume","")+"\\r\\n\\r\\n";
    body+="Why consider the applicant:\\r\\n"+d.value("message","")+"\\r\\n";
    std::string payload="From: CodeNova Careers <"+user+">\\r\\nTo: "+to+"\\r\\nSubject: New CS Job Application - "+d.value("position","")+"\\r\\nContent-Type: text/plain; charset=utf-8\\r\\n\\r\\n"+body;
    size_t pos=0;
    CURL* c=curl_easy_init(); if(!c){err="curl init failed";return false;}
    curl_easy_setopt(c,CURLOPT_URL,smtp.c_str());
    curl_easy_setopt(c,CURLOPT_USERNAME,user.c_str()); curl_easy_setopt(c,CURLOPT_PASSWORD,pass.c_str());
    curl_easy_setopt(c,CURLOPT_USE_SSL,(long)CURLUSESSL_ALL); curl_easy_setopt(c,CURLOPT_MAIL_FROM,user.c_str());
    struct curl_slist* rcpt=nullptr; rcpt=curl_slist_append(rcpt,to.c_str()); curl_easy_setopt(c,CURLOPT_MAIL_RCPT,rcpt);
    curl_easy_setopt(c,CURLOPT_READFUNCTION,+[](char* ptr,size_t size,size_t nmemb,void* userdata)->size_t{auto* s=(std::string*)userdata;size_t n=size*nmemb;size_t left=s->size();size_t take=left<n?left:n;memcpy(ptr,s->data(),take);s->erase(0,take);return take;});
    std::string data=payload; curl_easy_setopt(c,CURLOPT_READDATA,&data); curl_easy_setopt(c,CURLOPT_UPLOAD,1L);
    CURLcode r=curl_easy_perform(c); if(r!=CURLE_OK)err=curl_easy_strerror(r);
    curl_slist_free_all(rcpt); curl_easy_cleanup(c); return r==CURLE_OK;
}
int main(){curl_global_init(CURL_GLOBAL_DEFAULT);httplib::Server svr;svr.set_mount_point("/","./public");svr.Get("/health",[](const httplib::Request&,httplib::Response& res){res.set_content(R"({"ok":true,"service":"CodeNova Careers"})","application/json");});svr.Post("/apply",[](const httplib::Request& req,httplib::Response& res){try{auto d=json::parse(req.body);if(d.value("name","").empty()||d.value("email","").empty()||d.value("position","").empty()){res.status=400;res.set_content(R"({"error":"Name, email and position are required"})","application/json");return;}std::string err;if(!send_mail(d,err)){res.status=500;res.set_content(json({{"error",err}}).dump(),"application/json");return;}res.set_content(R"({"ok":true})","application/json");}catch(...){res.status=400;res.set_content(R"({"error":"Invalid request"})","application/json");}});std::cout<<"CodeNova server: http://localhost:8080\n";svr.listen("0.0.0.0",8080);curl_global_cleanup();}
