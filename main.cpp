
#include <iostream>
#include <sstream>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

// make && echo "hello world" 2>&1 | ./bin/dogcat 127.0.0.1 8124 -d -o "#domain:live,service:hello" -              

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		cout << "USAGE: dogcat HOST PORT OPTIONS... FILES..." << endl;
		cout << "EXAMPLE: echo \"hello world\" 2>&1 | ./dogcat 127.0.0.1 8124 -d -o \"#domain:live,service:hello\" -" << endl;
		return EXIT_FAILURE;
	}
	
	const char *const hostArg = argv[1];
	const char *const portArg = argv[2];
	
	int sock;
	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		cerr << "UDP socket failed" << endl;
		return EXIT_FAILURE;
	}
	
	sockaddr_in sockAddr = { 0 };
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = htons(atoi(portArg));
	if (inet_aton(hostArg, &sockAddr.sin_addr) == 0) 
	{
		cerr << "Host address invalid" << endl;
		return EXIT_FAILURE;
	}
	
	const char *title = "";
	bool wantTimestampAdded = false;
	const char *optionalOptions = "";
	
	int i;
	for (i = 3; i < argc; ++i)
	{
		if (strcmp(argv[i], "-d") == 0)
		{
			wantTimestampAdded = true;
		}
		else if ((strcmp(argv[i], "-o") == 0) && (i + 1) < argc)
		{
			++i;
			optionalOptions = argv[i];
		}
		else if ((strcmp(argv[i], "-t") == 0) && (i + 1) < argc)
		{
			++i;
			title = argv[i];
		}
		else
		{
			break;
		}
	}
	
	int titleLength = strlen(title);
	string messageStart;
	; {
		stringstream ss;
		ss << "_e{" << titleLength << ",";
		messageStart = ss.str();
	}
	
	for (; i < argc; ++i)
	{
		const char *const fileName = argv[i];
		int fd;
		if (strcmp(fileName, "-") == 0) fd = 0;
		else fd = open(fileName, O_RDONLY);
		if (fd < 0)
		{
			cerr << "Cannot open " << fileName << endl;
			continue;
		}
		char buf[256];
		buf[255] = '\x00';
		int bufFill = 0;
		int bufAdd;
		int bufLen;
		for (;;)
		{
			bufAdd = read(fd, &buf[bufFill], 255 - bufFill);
			if (bufAdd == 0)
			{
				bufAdd = -1;
			}
			bool endOfFile = bufAdd < 0;
			for (;;)
			{
				bool marker = false;
				if (bufAdd > 0)
				{
					bufLen = bufFill + bufAdd;
					for (int i = bufFill; i < bufLen; ++i)
					{
						if (buf[i] < ' ' || buf[i] == '\r' || buf[i] == '\n' /* || buf[i] == 'l' */)
						{
							bufLen = i + 1;
							marker = true;
							break;
						}
					}
					bufFill += bufAdd;
					// buf[bufFill] = 0; // TEST
				}
				else
				{
					marker = true;
					bufLen = bufFill + 1;
					bufFill = 0;
				}
				if (marker && (bufLen > 1) || bufLen >= 255)
				{
					stringstream ss;
					ss << messageStart << (bufLen - 1) << "}:" << title;
					ss << "|";
					ss.write(buf, bufLen - 1);
					if (wantTimestampAdded)
						ss << "|d:" << time(NULL);
					if (optionalOptions[0])
						ss << "|" << optionalOptions;
					ss << "\n";
					string s = ss.str();
					
					sendto(sock, s.c_str(), s.size(), 0 , (sockaddr *)&sockAddr, sizeof(sockaddr_in));
					// cout << s.c_str();
				}
				if (bufLen >= 255)
				{
					bufFill = 0;
					break;
				}
				else if (bufFill && bufLen && (bufLen != bufFill))
				{
					bufAdd = bufFill - bufLen;
					bufFill = 0;
					// cout << "buf STR >>>" << buf << "<<<" << endl;
					memcpy(buf, &buf[bufLen], bufAdd /* + 1 */);
					// cout << "buf END >>>" << buf << "<<<" << endl;
				}
				else
				{
					// cout << "buf RESET" << endl;
					if (marker)
					{
						bufFill = 0;
					}
					break;
				}
			}
			if (bufAdd < 0)
			{
				break;
			}
		}
		close(fd);
	}
	
	return EXIT_SUCCESS;
}

/* end of file */
