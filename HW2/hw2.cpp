#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <utility>
#include <sstream>
#include <regex>

using namespace std;
#define MAXLINE 1000

typedef struct DNS_HEADER {
    uint16_t id; // identification number 
    
    uint8_t qr : 1; // 0: query; 1: response 
    uint8_t opcode : 4; // purpose of message
    uint8_t aa : 1; // authoritive answer
    uint8_t tc : 1; // truncated message
    uint8_t rd : 1; // recursion desired

    uint8_t ra : 1; // recursion available
    uint8_t z : 3; // reserved
    uint8_t rcode : 4; // response code

    uint16_t qdcount; // number of question records
    uint16_t ancount; // number of answer records
    uint16_t nscount; // number of authority records
    uint16_t arcount; // number of additional records 
} __attribute__((packed)) dns_header;

typedef struct QUESTION {
    unsigned short qtype;
    unsigned short qclass;
} question;

typedef struct ResourceRecord {
    char name[10];
    uint32_t ttl;
    uint16_t rclass;
    uint16_t rtype;
    char rdata[500];
} rr;

// convert www.haha.com. to 3www4haha3com0
// return len(www.haha.com.)
int name2DNSname(char* dnsname, char* name) {
    u_int8_t len;
    int lencount = 0;
    char* token;
    char num[10];
    char toparse[100];
    strcpy(toparse, name);
    // 好拉直接把點點拿掉拉煩死了==
    toparse[strlen(toparse) - 1] = '\0';
    token = strtok(toparse, ".");
    while (token != NULL) {
        len = strlen(token);
        dnsname[lencount] = len;
        strcpy(dnsname + lencount + 1, token);
        lencount += len + 1;
        token = strtok(NULL, ".");
    }
    dnsname[lencount] = 0;

    return lencount;
}

// 3www2ha3com0 -> www.ha.com
// return len(3www2ha3com0)
int DNSname2name(char* name, char* dnsname) {
    int len, count = 0;
    len = dnsname[count];
    while (len != 0) {
        // modify "name"
        strncpy(name + count, dnsname + 1 + count, len);
        count += len;
        name[count] = '\0';
        strcat(name, ".");
        count += 1;

        // update len
        len = dnsname[count];
    }
    name[count - 1] = '.';
    name[count] = '\0';

    return count + 1;
}

int DNSnamelen(char* dnsname) {
    char haha[100];
    int len = DNSname2name(haha, dnsname);
    return len;
}

int namelen(char* name) {
    char haha[100];
    int len = name2DNSname(haha, name);
    return len;
}

int qtypeint(string str) {
    if (str == "A") {
        return 1;
    }
    else if (str == "AAAA") {
        return 28;
    }
    else if (str == "NS") {
        return 2;
    }
    else if (str == "CNAME") {
        return 5;
    }
    else if (str == "SOA") {
        return 6;
    }
    else if (str == "MX") {
        return 15;
    }
    else if (str == "TXT") {
        return 16;
    }
    return -1;
}

void dnamecpy(char* dst, char* src) {
    int pos = 0;
    while (src[pos]) {
        dst[pos] = src[pos];
        strncpy(dst + pos + 1, src + pos + 1, src[pos]);
        pos += 1 + src[pos];
    }
    dst[pos] = 0;
}

// @ + example1.org. = example1.org.; www + example1.org. = www.example1.org.
void getrrname(char* prefix, char* name) {
    if (strncmp(prefix, "@", 1) == 0) {
        return;
    }
    else {
        char temp[50];
        strcpy(temp, name);
        bzero(name, strlen(name) + 1);
        strcat(name, prefix);
        strcat(name, ".");
        strcat(name, temp);
    }
}

bool is_number(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

bool canparse(char* str, char* buf) {
    char temp[100];
    
    strcpy(temp, str);
    // 點點變不見
    for (int i = 0; i < strlen(temp); i++) {
        if (temp[i] == '.') {
            temp[i] = ' ';
        }
    }
    string haha = string(temp);
    stringstream ss(haha);
    string hoho;

    for (int i = 0; i < 4; i++) {
        ss >> hoho;
        if (!is_number(hoho)) {
            buf = NULL;
            return false;
        }
        buf[i] = stoi(hoho);
    }
    return true;
}

// response=0 for query; response=1 for response
// all ints should be in host byte order
int fill_header(char* dst, char* recvbuf, int response, int qdcount, int ancount, int nscount, int arcount) {
    memcpy(dst, recvbuf, sizeof(dns_header));
    dns_header* dhs_ptr = (dns_header*) dst;

    // fill in header
    dhs_ptr->qr = htons(response);
    dhs_ptr->ra = 1;

    dhs_ptr->qdcount = htons(qdcount);
    dhs_ptr->ancount = htons(ancount);
    dhs_ptr->nscount = htons(nscount);
    dhs_ptr->arcount = htons(arcount);

    return sizeof(dns_header);
}

int fill_ques(char* dst, char* qname, question* ques) {
    int qnamelen = DNSnamelen(qname);
    // qname
    dnamecpy(dst, qname);
    // question
    memcpy(dst + qnamelen, ques, sizeof(question));

    return qnamelen + sizeof(question);
}

// rtype, rclass, ttl, rdlen should all be network byte order
int fill_rr(char* dst, char* name, u_int16_t* rtype, u_int16_t* rclass, unsigned int* ttl, uint16_t* rdlen, void* rdata) {
    int namelen = DNSnamelen(name);
    // name
    dnamecpy(dst, name);
    // type & class
    memcpy(dst + namelen, rtype, sizeof(u_int16_t));
    memcpy(dst + namelen + sizeof(uint16_t), rclass, sizeof(u_int16_t));
    // ttl
    memcpy(dst + namelen + sizeof(question), ttl, sizeof(unsigned int));
    // rdlen
    memcpy(dst + namelen + sizeof(question) + sizeof(unsigned int), rdlen, sizeof(uint16_t));
    // rdata
    memcpy(dst + namelen + sizeof(question) + sizeof(unsigned int) + sizeof(uint16_t), rdata, ntohs(*rdlen));

    return namelen + sizeof(question) + sizeof(unsigned int) + sizeof(uint16_t) + ntohs(*rdlen);
}

int fill_name(char* dst, char* ns_str) {
    char temp[100];
    bzero(temp, sizeof(temp));
    strcpy(temp, ns_str);
    char dnsname[100];
    bzero(dnsname, sizeof(dnsname));
    name2DNSname(dnsname, temp);
    int len = DNSnamelen(dnsname);
    memcpy(dst, dnsname, len);
    return len;
}

int fill_mx(char* dst, char* mx_str) {
    char temp[100];
    strcpy(temp, mx_str);
    string str = string(temp);
    stringstream ss(str);
    string num_str;
    uint16_t num;
    ss >> num_str;
    num = htons(stoi(num_str));
    memcpy(dst, &num, sizeof(uint16_t));
    string tempstr;
    ss >> tempstr;
    char dnsname[100], name[100];
    strcpy(name, tempstr.c_str());
    bzero(dnsname, sizeof(dnsname));
    name2DNSname(dnsname, name);
    int namelen = DNSnamelen(dnsname);
    memcpy(dst + sizeof(uint16_t), dnsname, namelen);

    return namelen + sizeof(uint16_t);
}

int fill_soa(char* dst, char* soa_str) {
    char mname[50], rname[50];
    uint32_t serial, refresh, retry, expire, minimum;
    int mnamelen, rnamelen;
    // retrieve soa data

    string _soa_str = string(soa_str);
    stringstream ss(_soa_str);
    char temp[100];
    string temp_str;

    ss >> temp_str;
    bzero(mname, sizeof(mname));
    bzero(temp, sizeof(temp));
    strcpy(temp, temp_str.c_str());
    name2DNSname(mname, temp);
    mnamelen = DNSnamelen(mname);

    ss >> temp_str;
    bzero(rname, sizeof(rname));
    bzero(temp, sizeof(temp));
    strcpy(temp, temp_str.c_str());
    name2DNSname(rname, temp);
    rnamelen = DNSnamelen(rname);

    ss >> temp_str;
    serial = htonl(stoi(temp_str));

    ss >> temp_str;
    refresh = htonl(stoi(temp_str));

    ss >> temp_str;
    retry = htonl(stoi(temp_str));

    ss >> temp_str;
    expire = htonl(stoi(temp_str));

    ss >> temp_str;
    minimum = htonl(stoi(temp_str));
    
    // fill soa
    int pos = 0;
    memcpy(dst + pos, mname, mnamelen);
    pos += mnamelen;
    memcpy(dst + pos, rname, rnamelen);
    pos += rnamelen;
    memcpy(dst + pos, &serial, sizeof(uint32_t));
    pos += sizeof(uint32_t);
    memcpy(dst + pos, &refresh, sizeof(uint32_t));
    pos += sizeof(uint32_t);
    memcpy(dst + pos, &retry, sizeof(uint32_t));
    pos += sizeof(uint32_t);
    memcpy(dst + pos, &expire, sizeof(uint32_t));
    pos += sizeof(uint32_t);
    memcpy(dst + pos, &minimum, sizeof(uint32_t));
    pos += sizeof(uint32_t);

    return pos;
}

// return if domain1 is subdomain of domain2
bool subdomain(const char* domain1, const char* domain2) {
    return string(domain1).find(string(domain2)) != string::npos;
}

int main(int argc, char **argv) {
    int					sockfd, forsockfd;
	int	 				optval;
	struct sockaddr_in	cliaddr, servaddr, fns_addr;
	char			    sendbuf[5000], recvbuf[5000];
    int                 n;
    int                 packetlen;
    socklen_t           len;
    char                name[100], dnsname[100];
    int                 nnamelen, dnamelen;
    FILE *              fptr;
    fstream             fs, fsz;
    string              str;
    int authcount = 0;

    // read file
    fs.open(argv[2], std::fstream::in);
    // get foreign name server ip
    fs >> str;
    memset(&fns_addr, 0, sizeof(fns_addr)); 
    inet_pton(AF_INET, str.c_str(), &(fns_addr.sin_addr.s_addr));

    map<string, vector<rr> > dms;
    string token, domain_str;
    int pos;
    while (!fs.eof()) {
        fs >> str;
        // parse <domain>,<path>
        pos = str.find(",");
        domain_str = str.substr(0, pos);
        str.erase(0, pos + string(",").length());
        fsz.open(str.c_str(), fstream::in);

        getline(fsz, str);
        // read rrs
        // convert to network byte order on reading
        while (!fsz.eof()) {
            rr dummy;
            getline(fsz, str);
            // parse <NAME>,<TTL>,<CLASS>,<TYPE>,<RDATA>
            pos = str.find(",");
            token = str.substr(0, pos);
            strcpy(dummy.name, token.c_str());
            str.erase(0, pos + string(",").length());

            pos = str.find(",");
            token = str.substr(0, pos);
            dummy.ttl = htonl(stoi(token));
            str.erase(0, pos + string(",").length());

            pos = str.find(",");
            token = str.substr(0, pos);
            dummy.rclass = htons(1); // 應該都是IN吧
            str.erase(0, pos + string(",").length());

            pos = str.find(",");
            token = str.substr(0, pos);
            dummy.rtype = htons(qtypeint(token));
            str.erase(0, pos + string(",").length());

            strcpy(dummy.rdata, str.c_str());

            dms[domain_str].push_back(dummy);
        }
        fsz.close();
    }

    // dns client
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    optval = 1;

    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr)); 

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    servaddr.sin_family    = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(atoi(argv[1]));

    bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr));

    // foreign name server
    forsockfd = socket(AF_INET, SOCK_DGRAM, 0);

    fns_addr.sin_family = AF_INET;
    fns_addr.sin_port = htons(53);

    len = sizeof(cliaddr); 
    dns_header header;
    dns_header* dh_ptr;
    question* q_ptr;

    while (1) {
        packetlen = 0;
        bzero(recvbuf, sizeof(recvbuf));
        n = recvfrom(sockfd, recvbuf, MAXLINE, 0, ( struct sockaddr *) &cliaddr, &len);
        if (n < 0) {
            perror("recving dns request error: ");
        }

        // get domain
        bzero(name, sizeof(name));
        DNSname2name(name, recvbuf + sizeof(dns_header));

        pair<string, vector<rr> > sr_pair;
        // rr* found_rr;
        int found = 0;
        int exist = 0;
        // go through all domains
        for (auto domain : dms) {
            if (domain.first == string(name)) { // recv domain
                sr_pair = domain;
                found = 2;
                break;
            }
            else if (subdomain(name, domain.first.c_str())) {
                sr_pair = domain;
                found = 1;
                break;
            }
        }

        // send response
        bzero(sendbuf, sizeof(sendbuf));
        dnamelen = DNSnamelen(recvbuf + sizeof(dns_header));
        int add_sec_offset = sizeof(dns_header) + dnamelen + sizeof(question);
        q_ptr = (question*) (recvbuf + sizeof(dns_header) + dnamelen);
        authcount = 0;
        if (found == 2) { // recv domain
            vector<rr> records;
            // go through the entire zone file, retrieve all entries with same type
            for (auto record : sr_pair.second) {
                if (strncmp(record.name, "@", 1) == 0 && record.rtype == q_ptr->qtype) {
                    records.push_back(record);
                }
            }

            if (records.size() > 0) { // found some entries
                if ((ntohs(q_ptr->qtype) == 5) // CNAME
                    || (ntohs(q_ptr->qtype) == 16)) { // TXT
                    // packet structure: header + question_sec + author_sec (ns) + addition_sec (抄來的)
                    packetlen += fill_header(sendbuf + packetlen, recvbuf, 1, 1, records.size(), 1, 1);
                    packetlen += fill_ques(sendbuf + packetlen, recvbuf + packetlen, (question*) (recvbuf + packetlen + dnamelen));
                    
                    // answer section
                    for (int k = 0; k < records.size(); k++) {
                        if (ntohs(q_ptr->qtype) == 5) { // CNAME
                            char name_temp[100], dname_temp[100];
                            strcpy(name_temp, sr_pair.first.c_str());
                            name2DNSname(dname_temp, name_temp);

                            char cname_temp[100];
                            uint16_t cname_len = fill_name(cname_temp, records[k].rdata);
                            cname_len = htons(cname_len);
                            packetlen += fill_rr(sendbuf + packetlen, dname_temp, &records[k].rtype, &records[k].rclass, &records[k].ttl, &cname_len, cname_temp);
                        }
                        else if (ntohs(q_ptr->qtype) == 16) { // TXT
                            char name_temp[100], dname_temp[100];
                            strcpy(name_temp, sr_pair.first.c_str());
                            name2DNSname(dname_temp, name_temp);
                            records[k].rdata[strlen(records[k].rdata) - 1] = '\0';
                            uint16_t rdata_len = htons(strlen(records[k].rdata) + 1);
                            char txttosend[100];
                            strcpy(txttosend + 1, records[k].rdata);
                            txttosend[0] = strlen(records[k].rdata);
                            packetlen += fill_rr(sendbuf + packetlen, dname_temp, &records[k].rtype, &records[k].rclass, &records[k].ttl, &rdata_len, txttosend);
                        }
                    }
                    // authority section (NS)
                    for (auto record : sr_pair.second) {
                        if (record.rtype == htons(2)) { // NS
                            authcount++;
                            char name_temp[100], dname_temp[100];
                            strcpy(name_temp, sr_pair.first.c_str());
                            name2DNSname(dname_temp, name_temp);

                            char ns_temp[100];
                            uint16_t ns_len = fill_name(ns_temp, record.rdata);
                            ns_len = htons(ns_len);
                            packetlen += fill_rr(sendbuf + packetlen, dname_temp, &record.rtype, &record.rclass, &record.ttl, &ns_len, ns_temp);
                        }
                    }
                    fill_header(sendbuf, recvbuf, 1, 1, records.size(), authcount, 1);

                    // additional section
                    memcpy(sendbuf + packetlen, recvbuf + add_sec_offset, n - add_sec_offset);
                    packetlen += n - add_sec_offset;

                    sendto(sockfd, sendbuf, packetlen, 0, (struct sockaddr *)&cliaddr, len);
                }
                else if (ntohs(q_ptr->qtype) == 15) { // MX: 1112
                    // packet structure: header + question_sec + author_sec (ns) + addition_sec * 2 (subdomain: ip + 抄來的)
                    packetlen += fill_header(sendbuf + packetlen, recvbuf, 1, 1, records.size(), 1, 2);
                    packetlen += fill_ques(sendbuf + packetlen, recvbuf + packetlen, (question*) (recvbuf + packetlen + dnamelen));
                    
                    // answer section
                    for (int k = 0; k < records.size(); k++) {
                        char name_temp[100], dname_temp[100];
                        strcpy(name_temp, sr_pair.first.c_str());
                        name2DNSname(dname_temp, name_temp);

                        char mx_temp[100];
                        uint16_t mx_len = fill_mx(mx_temp, records[k].rdata);
                        mx_len = htons(mx_len);
                        packetlen += fill_rr(sendbuf + packetlen, dname_temp, &records[k].rtype, &records[k].rclass, &records[k].ttl, &mx_len, mx_temp);
                    }

                    // authority section (NS)
                    for (auto record : sr_pair.second) {
                        if (record.rtype == htons(2)) { // NS
                            authcount++;
                            char name_temp[100], dname_temp[100];
                            strcpy(name_temp, sr_pair.first.c_str());
                            name2DNSname(dname_temp, name_temp);

                            char ns_temp[100];
                            uint16_t ns_len = fill_name(ns_temp, record.rdata);
                            ns_len = htons(ns_len);
                            packetlen += fill_rr(sendbuf + packetlen, dname_temp, &record.rtype, &record.rclass, &record.ttl, &ns_len, ns_temp);
                        }
                    }

                    // additional section
                    for (auto record : sr_pair.second) {
                        if (strncmp(record.name, "mail", 4) == 0) {
                            // get mail.{domain}
                            char temp[100];
                            strcpy(temp, sr_pair.first.c_str());
                            getrrname(record.name, temp);

                            // mail.{domain} -> 4mail...0
                            char name_temp[100], dname_temp[100];
                            strcpy(name_temp, temp);
                            name2DNSname(dname_temp, name_temp);

                            uint16_t A_addr_len = htons(4);
                            uint32_t A_addr;
                            A_addr = inet_addr(record.rdata);
                            packetlen += fill_rr(sendbuf + packetlen, dname_temp, &record.rtype, &record.rclass, &record.ttl, &A_addr_len, &A_addr);
                            break;
                        }
                    }
                    fill_header(sendbuf, recvbuf, 1, 1, records.size(), authcount, 2);

                    memcpy(sendbuf + packetlen, recvbuf + add_sec_offset, n - add_sec_offset);
                    packetlen += n - add_sec_offset;
                    
                    sendto(sockfd, sendbuf, packetlen, 0, (struct sockaddr *)&cliaddr, len);
                }
                else if (ntohs(q_ptr->qtype) == 2) { // NS: 1102
                    // packet structure: header + question_sec + addition_sec * 2 (subdomain: ip + 抄來的)
                    packetlen += fill_header(sendbuf + packetlen, recvbuf, 1, 1, records.size(), 0, 2);
                    packetlen += fill_ques(sendbuf + packetlen, recvbuf + packetlen, (question*) (recvbuf + packetlen + dnamelen));
                    
                    // answer section
                    for (int k = 0; k < records.size(); k++) {
                        char name_temp[100], dname_temp[100];
                        strcpy(name_temp, sr_pair.first.c_str());
                        name2DNSname(dname_temp, name_temp);

                        char ns_temp[100];
                        uint16_t ns_len = fill_name(ns_temp, records[k].rdata);
                        ns_len = htons(ns_len);
                        packetlen += fill_rr(sendbuf + packetlen, dname_temp, &records[k].rtype, &records[k].rclass, &records[k].ttl, &ns_len, ns_temp);
                    }

                    // additional section
                    for (auto record : sr_pair.second) {
                        if (strncmp(record.name, "dns", 3) == 0) {
                            // get mail.{domain}
                            char temp[100];
                            strcpy(temp, sr_pair.first.c_str());
                            getrrname(record.name, temp);

                            // mail.{domain} -> 4mail...0
                            char name_temp[100], dname_temp[100];
                            strcpy(name_temp, temp);
                            name2DNSname(dname_temp, name_temp);

                            uint16_t A_addr_len = htons(4);
                            uint32_t A_addr;
                            A_addr = inet_addr(record.rdata);
                            packetlen += fill_rr(sendbuf + packetlen, dname_temp, &record.rtype, &record.rclass, &record.ttl, &A_addr_len, &A_addr);
                            break;
                        }
                    }

                    memcpy(sendbuf + packetlen, recvbuf + add_sec_offset, n - add_sec_offset);
                    packetlen += n - add_sec_offset;
                    
                    sendto(sockfd, sendbuf, packetlen, 0, (struct sockaddr *)&cliaddr, len);
                }
                else if (ntohs(q_ptr->qtype) == 6) { // SOA
                    // packet structure: header + question_sec + answer + author_sec (ns) + addition_sec (抄來的)
                    packetlen += fill_header(sendbuf + packetlen, recvbuf, 1, 1, records.size(), 1, 1);
                    packetlen += fill_ques(sendbuf + packetlen, recvbuf + packetlen, (question*) (recvbuf + packetlen + dnamelen));
                    
                    // answer section (SOA)
                    for (auto record : sr_pair.second) {
                        if (record.rtype == htons(6)) { // SOA
                            char name_temp[100], dname_temp[100];
                            bzero(name_temp, sizeof(name_temp));
                            bzero(dname_temp, sizeof(dname_temp));
                            strcpy(name_temp, name);
                            getrrname(record.name, name_temp);
                            name2DNSname(dname_temp, name_temp);

                            char soa_temp[100];
                            uint16_t soa_len = fill_soa(soa_temp, record.rdata);
                            soa_len = htons(soa_len);
                            packetlen += fill_rr(sendbuf + packetlen, dname_temp, &record.rtype, &record.rclass, &record.ttl, &soa_len, soa_temp);
                            break;
                        }
                    }

                    // author section (NS)
                    for (auto record : sr_pair.second) {
                        if (record.rtype == htons(2)) { // NS
                            authcount++;
                            char name_temp[100], dname_temp[100];
                            strcpy(name_temp, sr_pair.first.c_str());
                            name2DNSname(dname_temp, name_temp);

                            char ns_temp[100];
                            uint16_t ns_len = fill_name(ns_temp, record.rdata);
                            ns_len = htons(ns_len);
                            packetlen += fill_rr(sendbuf + packetlen, dname_temp, &record.rtype, &record.rclass, &record.ttl, &ns_len, ns_temp);
                        }
                    }

                    memcpy(sendbuf + packetlen, recvbuf + add_sec_offset, n - add_sec_offset);
                    packetlen += n - add_sec_offset;

                    fill_header(sendbuf, recvbuf, 1, 1, records.size(), authcount, 1);
                    
                    sendto(sockfd, sendbuf, packetlen, 0, (struct sockaddr *)&cliaddr, len);
                }
                else { // A/AAAA丟水溝
                    cout << "hehe not implemented haha";
                }
            }
            else { // not found
                // packet structure: header + question_sec + author_sec + addition_sec
                // fill header & ques
                packetlen += fill_header(sendbuf + packetlen, recvbuf, 1, 1, 0, 1, 1);
                packetlen += fill_ques(sendbuf + packetlen, recvbuf + packetlen, (question*) (recvbuf + packetlen + dnamelen));

                // fill SOA
                for (auto record : sr_pair.second) {
                    if (record.rtype == htons(6)) { // SOA
                        char name_temp[100], dname_temp[100];
                        bzero(name_temp, sizeof(name_temp));
                        bzero(dname_temp, sizeof(dname_temp));
                        strcpy(name_temp, name);
                        getrrname(record.name, name_temp);
                        name2DNSname(dname_temp, name_temp);

                        char soa_temp[100];
                        uint16_t soa_len = fill_soa(soa_temp, record.rdata);
                        soa_len = htons(soa_len);
                        packetlen += fill_rr(sendbuf + packetlen, dname_temp, &record.rtype, &record.rclass, &record.ttl, &soa_len, soa_temp);
                        break;
                    }
                }

                // additional section
                memcpy(sendbuf + packetlen, recvbuf + add_sec_offset, n - add_sec_offset);
                packetlen += n - add_sec_offset;
                sendto(sockfd, sendbuf, packetlen, 0, (struct sockaddr *)&cliaddr, len);
            }

        }
        else if (found == 1) {
            // nip.io like service

            uint32_t nipioaddr;
            if (canparse(name, (char*)&nipioaddr)) {
                // fill header & ques
                packetlen += fill_header(sendbuf + packetlen, recvbuf, 1, 1, 1, 1, 1);
                packetlen += fill_ques(sendbuf + packetlen, recvbuf + packetlen, (question*) (recvbuf + packetlen + dnamelen));

                // fill answer
                char name_temp[100], dname_temp[100];
                strcpy(name_temp, name);
                name2DNSname(dname_temp, name_temp);
                uint16_t A_addr_len = htons(4);
                uint16_t dummyoneone = htons(1);
                uint32_t dummyoneoneoneone = htonl(1);
                packetlen += fill_rr(sendbuf + packetlen, dname_temp, &dummyoneone, &dummyoneone, &dummyoneoneoneone, &A_addr_len, &nipioaddr);

                // fill author (NS)
                for (auto record : sr_pair.second) {
                    if (record.rtype == htons(2)) { // NS
                        authcount++;
                        char name_temp[100], dname_temp[100];
                        strcpy(name_temp, sr_pair.first.c_str());
                        name2DNSname(dname_temp, name_temp);

                        char ns_temp[100];
                        uint16_t ns_len = fill_name(ns_temp, record.rdata);
                        ns_len = htons(ns_len);
                        packetlen += fill_rr(sendbuf + packetlen, dname_temp, &record.rtype, &record.rclass, &record.ttl, &ns_len, ns_temp);
                    }
                }

                // additional section
                memcpy(sendbuf + packetlen, recvbuf + add_sec_offset, n - add_sec_offset);
                packetlen += n - add_sec_offset;

                fill_header(sendbuf, recvbuf, 1, 1, 1, authcount, 1);

                sendto(sockfd, sendbuf, packetlen, 0, (struct sockaddr *)&cliaddr, len);
            }
            // go through the entire zone file, retrieve all matches
            vector<rr> records;
            for (auto record : sr_pair.second) {
                if (ntohs(record.rtype) == ntohs(q_ptr->qtype)) { // of same type
                    char temp[100];
                    strcpy(temp, sr_pair.first.c_str());
                    getrrname(record.name, temp);
                    if (strcmp(temp, name) == 0) { // of same name
                        records.push_back(record);
                    }
                }
            }

            if (records.size() > 0) {
                // 基本上只會是A跟AAAA
                if (ntohs(q_ptr->qtype) == 1 || ntohs(q_ptr->qtype) == 28) {
                    // packet structure: header + question_sec + ans_sec * N + author_sec(NS) + addition_sec(抄來的)
                    // fill header & ques
                    packetlen += fill_header(sendbuf + packetlen, recvbuf, 1, 1, records.size(), 1, 1);
                    packetlen += fill_ques(sendbuf + packetlen, recvbuf + packetlen, (question*) (recvbuf + packetlen + dnamelen));

                    // fill answer
                    for (int k = 0; k < records.size(); k++) {
                        if (q_ptr->qtype == htons(1)) { // A
                            char name_temp[100], dname_temp[100];
                            strcpy(name_temp, name);
                            name2DNSname(dname_temp, name_temp);
                            uint16_t A_addr_len = htons(4);
                            uint32_t A_addr;
                            A_addr = inet_addr(records[k].rdata);
                            packetlen += fill_rr(sendbuf + packetlen, dname_temp, &records[k].rtype, &records[k].rclass, &records[k].ttl, &A_addr_len, &A_addr);
                        }
                        else if (q_ptr->qtype == htons(28)) { // AAAA
                            char name_temp[100], dname_temp[100];
                            strcpy(name_temp, name);
                            name2DNSname(dname_temp, name_temp);
                            uint16_t AAAA_addr_len = htons(16);
                            char AAAA_addr[16];
                            records[k].rdata[strlen(records[k].rdata) - 1] = '\0';
                            int a = inet_pton(AF_INET6, records[k].rdata, AAAA_addr);
                            packetlen += fill_rr(sendbuf + packetlen, dname_temp, &records[k].rtype, &records[k].rclass, &records[k].ttl, &AAAA_addr_len, AAAA_addr);
                        }
                    }

                    // fill author (NS)
                    for (auto record : sr_pair.second) {
                        if (record.rtype == htons(2)) { // NS
                            authcount++;
                            char name_temp[100], dname_temp[100];
                            strcpy(name_temp, sr_pair.first.c_str());
                            name2DNSname(dname_temp, name_temp);

                            char ns_temp[100];
                            uint16_t ns_len = fill_name(ns_temp, record.rdata);
                            ns_len = htons(ns_len);
                            packetlen += fill_rr(sendbuf + packetlen, dname_temp, &record.rtype, &record.rclass, &record.ttl, &ns_len, ns_temp);
                        }
                    }

                    // additional section
                    memcpy(sendbuf + packetlen, recvbuf + add_sec_offset, n - add_sec_offset);
                    packetlen += n - add_sec_offset;

                    fill_header(sendbuf, recvbuf, 1, 1, records.size(), authcount, 1);

                    sendto(sockfd, sendbuf, packetlen, 0, (struct sockaddr *)&cliaddr, len);
                }
                else if (ntohs(q_ptr->qtype) == 5) {
                    // packet structure: header + question_sec + author_sec (ns) + addition_sec (抄來的)
                    packetlen += fill_header(sendbuf + packetlen, recvbuf, 1, 1, records.size(), 1, 1);
                    packetlen += fill_ques(sendbuf + packetlen, recvbuf + packetlen, (question*) (recvbuf + packetlen + dnamelen));
                    
                    // answer section
                    for (int k = 0; k < records.size(); k++) {
                        char name_temp[100], dname_temp[100];
                        strcpy(name_temp, sr_pair.first.c_str());
                        name2DNSname(dname_temp, name_temp);

                        char cname_temp[100];
                        uint16_t cname_len = fill_name(cname_temp, records[k].rdata);
                        cname_len = htons(cname_len);
                        packetlen += fill_rr(sendbuf + packetlen, dname_temp, &records[k].rtype, &records[k].rclass, &records[k].ttl, &cname_len, cname_temp);
                    }
                    // authority section (NS)
                    for (auto record : sr_pair.second) {
                        if (record.rtype == htons(2)) { // NS
                            authcount++;
                            char name_temp[100], dname_temp[100];
                            strcpy(name_temp, sr_pair.first.c_str());
                            name2DNSname(dname_temp, name_temp);

                            char ns_temp[100];
                            uint16_t ns_len = fill_name(ns_temp, record.rdata);
                            ns_len = htons(ns_len);
                            packetlen += fill_rr(sendbuf + packetlen, dname_temp, &record.rtype, &record.rclass, &record.ttl, &ns_len, ns_temp);
                        }
                    }
                    fill_header(sendbuf, recvbuf, 1, 1, records.size(), authcount, 1);

                    // additional section
                    memcpy(sendbuf + packetlen, recvbuf + add_sec_offset, n - add_sec_offset);
                    packetlen += n - add_sec_offset;

                    sendto(sockfd, sendbuf, packetlen, 0, (struct sockaddr *)&cliaddr, len);
                }
            }
            else { // not exist
                // packet structure: header + question_sec + author_sec + addition_sec
                // fill header & ques
                packetlen += fill_header(sendbuf + packetlen, recvbuf, 1, 1, 0, 1, 1);
                packetlen += fill_ques(sendbuf + packetlen, recvbuf + packetlen, (question*) (recvbuf + packetlen + dnamelen));

                // fill SOA
                for (auto record : sr_pair.second) {
                    if (record.rtype == htons(6)) { // SOA
                        char name_temp[100], dname_temp[100];
                        bzero(name_temp, sizeof(name_temp));
                        bzero(dname_temp, sizeof(dname_temp));
                        strcpy(name_temp, name);
                        getrrname(record.name, name_temp);
                        name2DNSname(dname_temp, name_temp);

                        char soa_temp[100];
                        uint16_t soa_len = fill_soa(soa_temp, record.rdata);
                        soa_len = htons(soa_len);
                        packetlen += fill_rr(sendbuf + packetlen, dname_temp, &record.rtype, &record.rclass, &record.ttl, &soa_len, soa_temp);
                        break;
                    }
                }

                // additional section
                memcpy(sendbuf + packetlen, recvbuf + add_sec_offset, n - add_sec_offset);
                packetlen += n - add_sec_offset;
                sendto(sockfd, sendbuf, packetlen, 0, (struct sockaddr *)&cliaddr, len);
            }
        }
        else if (!found) { // domain not found in 
            // send query to foreign dns server
            memcpy(sendbuf, recvbuf, MAXLINE);
            dh_ptr = (dns_header*)sendbuf;
            dh_ptr->ra = 1;
            sendto(forsockfd, sendbuf, n, 0, (struct sockaddr *)&fns_addr, len);

            // recv foreign dns server's response
            bzero(recvbuf, sizeof(recvbuf));
            n = recvfrom(forsockfd, recvbuf, MAXLINE, 0, ( struct sockaddr *) &fns_addr, &len);

            // send back to client
            sendto(sockfd, recvbuf, n, 0, (struct sockaddr *)&cliaddr, len);
        }
    }

    return 0; 
}