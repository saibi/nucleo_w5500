#include "stm32f7xx_hal.h"
#include "ftpc.h"

un_l2cval remote_ip;
uint16_t  remote_port;
un_l2cval local_ip;
uint16_t  local_port;
uint8_t connect_state_control_ftpc = 0;
uint8_t connect_state_data_ftpc = 0;
uint8_t gModeActivePassiveflag = 0;
uint8_t FTP_destip[4] = {172, 10, 150, 130};	// For FTP client examples; destination network info
uint16_t FTP_destport = 21;						// For FTP client examples; destination network info
uint8_t gMenuStart = 0;
uint8_t gDataSockReady = 0;
uint8_t gDataPutGetStart = 0;
static uint8_t gMsgBuf[20]={0,};

uint8_t seg_files[9]={'s','e','g','0','.','t','x','t',};
uint8_t seg_index = 0;

struct ftpc ftpc;
struct Command Command;

extern UART_HandleTypeDef huart3;

void ftpc_init(uint8_t * src_ip)
{
	long ret = 0;
	ftpc.dsock_mode = ACTIVE_MODE;

	local_ip.cVal[0] = src_ip[0];
	local_ip.cVal[1] = src_ip[1];
	local_ip.cVal[2] = src_ip[2];
	local_ip.cVal[3] = src_ip[3];
	local_port = 35000;
	strcpy(ftpc.workingdir, "/");
	ret = socket(CTRL_SOCK, Sn_MR_TCP, FTP_destport, 0x0);
	if(ret != CTRL_SOCK) printf("%d:Init Connect error = %d \r\n",CTRL_SOCK,ret);

}
uint8_t ftpc_run(uint8_t * dbuf)
{
#ifndef Need_UARTGetCharBlocking_func
	uint16_t size = 0;
	long ret = 0;
	uint32_t send_byte, recv_byte;
	uint32_t blocklen;
	uint32_t remain_filesize;
	uint32_t remain_datasize;
	uint8_t msg_c;
	uint8_t dat[50]={0,};
	uint32_t totalSize = 0, availableSize = 0;
	uint16_t Responses;
	uint8_t data_sock_status;

    switch(getSn_SR(CTRL_SOCK))
    {
    	case SOCK_ESTABLISHED :
    		if(!connect_state_control_ftpc){
    			printf("%d:CTRL_SOCK FTP Connected\r\n", CTRL_SOCK);
    			strcpy(ftpc.workingdir, "/");
    			connect_state_control_ftpc = 1;
    		}

    		/////
			if((size = getSn_RX_RSR(CTRL_SOCK)) > 0){ // Don't need to check SOCKERR_BUSY because it doesn't not occur.
				memset(dbuf, 0, _MAX_SS);
				if(size > _MAX_SS) size = _MAX_SS - 1;
				ret = recv(CTRL_SOCK,dbuf,size);
				dbuf[ret] = '\0';
				if(ret != size)
				{
					if(ret==SOCK_BUSY) return 0;
					if(ret < 0){
						printf("%d:recv() error:%ld\r\n",CTRL_SOCK,ret);
						close(CTRL_SOCK);
						return ret;
					}
				}
#ifdef _FTPC_DEBUG_
				printf(">>C_SOCK RX CMD: %s\r\n", dbuf);
#endif
				//Responses =(dbuf[0]-'0')*100+(dbuf[1]-'0')*10+(dbuf[2]-'0');
				//if(Responses == R_200 )  break;
				proc_ftpc((char *)dbuf);

			}
    		/////////////////////

    		if(gMenuStart ){
				gMenuStart = 0;
				if(!seg_index)
				{
					printf("\r\n----------------------------------------\r\n");
					printf("Press menu key\r\n");
					printf("----------------------------------------\r\n");
					//printf("0> Input ID & Password\r\n");
					printf("1> View FTP Server Directory\r\n");
					printf("2> View My Directory\r\n");
					printf("3> Sets the type of file. Current state : %s\r\n", (ftpc.type==ASCII_TYPE)?"Ascii":"Binary");
					printf("4> Sets Data Connection. Current state : %s\r\n", (ftpc.dsock_mode==ACTIVE_MODE)?"Active":"Passive");
					printf("5> Put File to Server\r\n");
					printf("6> Get File from Server\r\n");
					printf("t> Get Text Files from Server\r\n");
		#if defined(F_FILESYSTEM)
					printf("7> Delete My File\r\n");
		#endif
					printf("----------------------------------------\r\n");
				}

				while(1){

					if(seg_index == 0)
						msg_c=ftp_getc();
					else
					{
						msg_c = 't';
					}

				 if(msg_c=='1'){
						if(ftpc.dsock_mode==PASSIVE_MODE){
							sprintf(dat,"PASV\r\n");
							send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
							Command.First = f_dir;
							break;
						}
						else{
							wiz_NetInfo gWIZNETINFO;
							ctlnetwork(CN_GET_NETINFO, (void*) &gWIZNETINFO);
							sprintf(dat,"PORT %d,%d,%d,%d,%d,%d\r\n", gWIZNETINFO.ip[0], gWIZNETINFO.ip[1], gWIZNETINFO.ip[2], gWIZNETINFO.ip[3], (uint8_t)(local_port>>8), (uint8_t)(local_port&0x00ff));
							send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
							Command.First = f_dir;
							gModeActivePassiveflag = 1;
							break;
						}
						break;
					}
					else if(msg_c=='5'){
						if(ftpc.dsock_mode==PASSIVE_MODE){
							sprintf(dat,"PASV\r\n");
							send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
							Command.First = f_put;
							break;
						}
						else{
							wiz_NetInfo gWIZNETINFO;
							ctlnetwork(CN_GET_NETINFO, (void*) &gWIZNETINFO);
							sprintf(dat,"PORT %d,%d,%d,%d,%d,%d\r\n", gWIZNETINFO.ip[0], gWIZNETINFO.ip[1], gWIZNETINFO.ip[2], gWIZNETINFO.ip[3], (uint8_t)(local_port>>8), (uint8_t)(local_port&0x00ff));
							send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
							Command.First = f_put;

							gModeActivePassiveflag = 1;
							break;
						}
					}
					else if(msg_c=='6'){
						if(ftpc.dsock_mode==PASSIVE_MODE){
							sprintf(dat,"PASV\r\n");
							send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
							Command.First = f_get;
							break;
						}
						else{
							wiz_NetInfo gWIZNETINFO;
							ctlnetwork(CN_GET_NETINFO, (void*) &gWIZNETINFO);
							sprintf(dat,"PORT %d,%d,%d,%d,%d,%d\r\n", gWIZNETINFO.ip[0], gWIZNETINFO.ip[1], gWIZNETINFO.ip[2], gWIZNETINFO.ip[3], (uint8_t)(local_port>>8), (uint8_t)(local_port&0x00ff));
							send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
							Command.First = f_get;

							gModeActivePassiveflag = 1;
							break;
						}
					}
					else if(msg_c=='t'){
						if(ftpc.dsock_mode==PASSIVE_MODE){
							sprintf(dat,"PASV\r\n");
							send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
							Command.First = f_get;
							break;
						}
						else{
							wiz_NetInfo gWIZNETINFO;
							ctlnetwork(CN_GET_NETINFO, (void*) &gWIZNETINFO);
							sprintf(dat,"PORT %d,%d,%d,%d,%d,%d\r\n", gWIZNETINFO.ip[0], gWIZNETINFO.ip[1], gWIZNETINFO.ip[2], gWIZNETINFO.ip[3], (uint8_t)(local_port>>8), (uint8_t)(local_port&0x00ff));
							send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
							Command.First = f_get_files;

							gModeActivePassiveflag = 1;
							break;
						}
					}
					else if(msg_c=='2'){
#if defined(F_FILESYSTEM)
						scan_files(ftpc.workingdir, dbuf, (int *)&size);
						printf("\r\n%s\r\n", dbuf);
#else
						if (strncmp(ftpc.workingdir, "/$Recycle.Bin", sizeof("/$Recycle.Bin")) != 0)
							size = sprintf(dbuf, "drwxr-xr-x 1 ftp ftp 0 Dec 31 2014 $Recycle.Bin\r\n-rwxr-xr-x 1 ftp ftp 512 Dec 31 2014 test.txt\r\n");
						printf("\r\n%s\r\n", dbuf);
#endif
						gMenuStart = 1;
						break;
					}
					else if(msg_c=='3'){
						printf("1> ASCII\r\n");
						printf("2> BINARY\r\n");
						while(1){
							msg_c=ftp_getc();
							if(msg_c=='1'){
								sprintf(dat,"TYPE %c\r\n", TransferAscii);
								ftpc.type = ASCII_TYPE;
								send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
								break;
							}
							else if(msg_c=='2'){
								sprintf(dat,"TYPE %c\r\n", TransferBinary);
								ftpc.type = IMAGE_TYPE;
								send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
								break;
							}
							else{
								printf("\r\nRetry  c=3 ...\r\n");
							}
						}
						break;
					}
					else if(msg_c=='4'){
						printf("1> ACTIVE\r\n");
						printf("2> PASSIVE\r\n");
						while(1){
							msg_c=ftp_getc();
							if(msg_c=='1'){
								ftpc.dsock_mode=ACTIVE_MODE;
								break;
							}
							else if(msg_c=='2'){
								ftpc.dsock_mode=PASSIVE_MODE;
								break;
							}
							else{
								printf("\r\nRetry c=4...\r\n");
							}
						}
						gMenuStart = 1;
						break;
					}
#if defined(F_FILESYSTEM)
					else if(msg_c=='7'){
						printf(">del filename?");
						sprintf(ftpc.filename, "/%s\r\n", User_Keyboard_MSG());
						if (f_unlink((const char *)ftpc.filename) != 0){
							printf("\r\nCould not delete.\r\n");
						}
						else{
							printf("\r\nDeleted.\r\n");
						}
						gMenuStart = 1;
						break;
					}
#endif
					else{
						printf("\r\nRetry  c=7 ...\r\n");
					}
				}
			}  //gMenuStart  End
			if(gDataSockReady){
				gDataSockReady = 0;
				switch(Command.First){
					case f_dir:
						printf("\r\nGet Remote file list\r\n");
						//sprintf(dat,"LIST\r\n");
						sprintf(dat,"NLST\r\n");
						send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
						break;
					case f_put:
						printf(">put file name?");
						fflush(stdout);
						sprintf(dat,"STOR %s\r\n", User_Keyboard_MSG());
						send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
						printf(">send %s \r\n",dat);
						break;
					case f_get:
						printf(">get file name?");
						fflush(stdout);
						sprintf(dat,"RETR %s\r\n", User_Keyboard_MSG());
						send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
						printf(">send %s \r\n",dat);
						break;
					case f_get_files:
#ifdef	_FTPC_DEBUG_
						printf(">get Text files index=%d\r\n",seg_index);
#endif
						//fflush(stdout);
						seg_files[3] = '0'+(seg_index % 10);
						seg_index++;
						if(seg_index >= 20)  seg_index = 0;
						sprintf(dat,"RETR %s\r\n",seg_files);
						send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
						printf(">send %s \r\n",dat);
						break;
					default:
						printf("Command.First = default\r\n");
						break;
				}
			}
			/*
    		if((size = getSn_RX_RSR(CTRL_SOCK)) > 0){ // Don't need to check SOCKERR_BUSY because it doesn't not occur.
    			memset(dbuf, 0, _MAX_SS);
    			if(size > _MAX_SS) size = _MAX_SS - 1;
    			ret = recv(CTRL_SOCK,dbuf,size);
    			dbuf[ret] = '\0';
    			if(ret != size)
    			{
    				if(ret==SOCK_BUSY) return 0;
    				if(ret < 0){
    					printf("%d:recv() error:%ld\r\n",CTRL_SOCK,ret);
    					close(CTRL_SOCK);
    					return ret;
    				}
    			}
    			printf("Rcvd Command: %s\r\n", dbuf);
    			proc_ftpc((char *)dbuf);
    		}  */
    		break;
    	case SOCK_CLOSE_WAIT :
    		printf("%d:CTRL_SOCK CloseWait\r\n",CTRL_SOCK);
    		if((ret=disconnect(CTRL_SOCK)) != SOCK_OK) return ret;
    		printf("%d:CTRL_SOCK Closed\r\n",CTRL_SOCK);
    		break;
    	case SOCK_CLOSED :
    		printf("%d:FTPStart\r\n",CTRL_SOCK);
    		if((ret=socket(CTRL_SOCK, Sn_MR_TCP, FTP_destport, 0x0)) != CTRL_SOCK){
    			printf("%d:socket() error:%ld\r\n", CTRL_SOCK, ret);
    			close(CTRL_SOCK);
    			return ret;
    		}
    		printf("%d:FTP Control Sock Opened\r\n",CTRL_SOCK);
    		break;
    	case SOCK_INIT :
    		printf("%d: Opened\r\n",CTRL_SOCK);
			if((ret = connect(CTRL_SOCK, FTP_destip, FTP_destport)) != SOCK_OK){
				printf("%d:Connect error =%d\r\n",CTRL_SOCK,ret);
				return ret;
			}
			connect_state_control_ftpc = 0;
			printf("%d:CTRL_SOCK Connectting...\r\n",CTRL_SOCK);
			break;
    	default :
    		break;
    }


    data_sock_status = getSn_SR(DATA_SOCK);
    switch(data_sock_status){
    	case SOCK_ESTABLISHED :
    	case SOCK_CLOSE_WAIT :
    		//printf("DATA SOCK_Estalished or Close wait.....gDataPutGetStart=%d...\r\n",gDataPutGetStart);
    		if(!connect_state_data_ftpc){
#ifdef	_FTPC_DEBUG_
    			printf("%d:FTP Data socket Connected\r\n", DATA_SOCK);
#endif
    			connect_state_data_ftpc = 1;
    		}
			if(gDataPutGetStart){
#ifdef	_FTPC_DEBUG_
				printf("gDataPutGet  Start.............in SOCK_ESTABLISHED\r\n");
#endif
				switch(Command.Second){
				case s_dir:
					printf("dir waiting...\r\n");
					if((size = getSn_RX_RSR(DATA_SOCK)) > 0){ // Don't need to check SOCKERR_BUSY because it doesn't not occur.
						printf("ok\r\n");
						memset(dbuf, 0, _MAX_SS);
						if(size > _MAX_SS) size = _MAX_SS - 1;
						ret = recv(DATA_SOCK,dbuf,size);
						dbuf[ret] = '\0';
						if(ret != size){
							if(ret==SOCK_BUSY) return 0;
							if(ret < 0){
								printf("%d:recv() error:%ld\r\n",CTRL_SOCK,ret);
								close(DATA_SOCK);
								return ret;
							}
						}
						printf("Rcvd Data:\n\r%s\n\r", dbuf);
						gDataPutGetStart = 0;
						Command.Second = s_nocmd;
					}
					break;
				case s_put:
					printf("put waiting...\r\n");
					if(strlen(ftpc.workingdir) == 1)
						sprintf(ftpc.filename, "/%s", (uint8_t *)gMsgBuf);
					else
						sprintf(ftpc.filename, "%s/%s", ftpc.workingdir, (uint8_t *)gMsgBuf);
#if defined(F_FILESYSTEM)
					ftpc.fr = f_open(&(ftpc.fil), (const char *)ftpc.filename, FA_READ);
					if(ftpc.fr == FR_OK){
						remain_filesize = ftpc.fil.fsize;
						printf("f_open return FR_OK\r\n");
						do{
							memset(dbuf, 0, _MAX_SS);
							if(remain_filesize > _MAX_SS)
								send_byte = _MAX_SS;
							else
								send_byte = remain_filesize;
							ftpc.fr = f_read(&(ftpc.fil), (void *)dbuf, send_byte , (UINT *)&blocklen);
							if(ftpc.fr != FR_OK){
								break;
							}
							printf("#");
							send(DATA_SOCK, dbuf, blocklen);
							remain_filesize -= blocklen;
						}while(remain_filesize != 0);
						printf("\r\nFile read finished\r\n");
						ftpc.fr = f_close(&(ftpc.fil));
					}
					else{
						printf("File Open Error: %d\r\n", ftpc.fr);
						ftpc.fr = f_close(&(ftpc.fil));
					}
#else
					remain_filesize = strlen(ftpc.filename);
					do{
						memset(dbuf, 0, _MAX_SS);
						blocklen = sprintf(dbuf, "%s", ftpc.filename);
						printf("########## dbuf:%s\r\n", dbuf);
						send(DATA_SOCK, dbuf, blocklen);
						remain_filesize -= blocklen;
					}while(remain_filesize != 0);
#endif
					gDataPutGetStart = 0;
					Command.Second = s_nocmd;
					disconnect(DATA_SOCK);
					break;
				case s_get:
				case s_get_files:
					printf("get waiting...\r\n");
					if(strlen(ftpc.workingdir) == 1)
						sprintf(ftpc.filename, "/%s", (uint8_t *)gMsgBuf);
					else
						sprintf(ftpc.filename, "%s/%s", ftpc.workingdir, (uint8_t *)gMsgBuf);
#if defined(F_FILESYSTEM)
					ftpc.fr = f_open(&(ftpc.fil), (const char *)ftpc.filename, FA_CREATE_ALWAYS | FA_WRITE);
					if(ftpc.fr == FR_OK){
						printf("f_open return FR_OK\r\n");
						while(1){
							if((remain_datasize = getSn_RX_RSR(DATA_SOCK)) > 0){
								while(1){
									memset(dbuf, 0, _MAX_SS);
									if(remain_datasize > _MAX_SS)	recv_byte = _MAX_SS;
									else	recv_byte = remain_datasize;
									ret = recv(DATA_SOCK, dbuf, recv_byte);
									ftpc.fr = f_write(&(ftpc.fil), (const void *)dbuf, (UINT)ret, (UINT *)&blocklen);
									remain_datasize -= blocklen;
									if(ftpc.fr != FR_OK){
										printf("f_write failed\r\n");
										break;
									}
									if(remain_datasize <= 0)	break;
								}
								if(ftpc.fr != FR_OK){
									printf("f_write failed\r\n");
									break;
								}
								printf("#");
							}
							else{
								if(getSn_SR(DATA_SOCK) != SOCK_ESTABLISHED)	break;
							}
						}
						printf("\r\nFile write finished\r\n");
						ftpc.fr = f_close(&(ftpc.fil));
						gDataPutGetStart = 0;
					}else{
						printf("File Open Error: %d\r\n", ftpc.fr);
					}
#else
					while(1){
						if((remain_datasize = getSn_RX_RSR(DATA_SOCK)) > 0){
							while(1){
								memset(dbuf, 0, _MAX_SS);
								if(remain_datasize > _MAX_SS)
									recv_byte = _MAX_SS;
								else
									recv_byte = remain_datasize;
								ret = recv(DATA_SOCK, dbuf, recv_byte);
								//printf("########## dbuf size=%d:\r\n%s\r\n",remain_datasize, dbuf);
								printf("########## dbuf size=%d:\r\n",remain_datasize);
								remain_datasize -= ret;
								if(remain_datasize <= 0)
									break;
							}
						}else{
							if(getSn_SR(DATA_SOCK) != SOCK_ESTABLISHED)
								break;
						}
					}
					gDataPutGetStart = 0;
					Command.Second = s_nocmd;
#endif
					break;
				default:
					printf("Command.Second = default\r\n");
					break;
				}
			}  // gDataPutGetStart
			if(data_sock_status == SOCK_CLOSE_WAIT)
			{
				printf("%d:CloseWait\r\n",DATA_SOCK);
				if((ret=disconnect(DATA_SOCK)) != SOCK_OK) return ret;
				printf("%d:Closed\r\n",DATA_SOCK);
			}
    		break;
    	/*
   		case SOCK_CLOSE_WAIT :
   			printf("%d:CloseWait\r\n",DATA_SOCK);

   			if(gDataPutGetStart){
				printf("gDataPutGet Start.............\r\n");
				switch(Command.Second){
				case s_dir:
					printf("dir waiting...\r\n");
					if((size = getSn_RX_RSR(DATA_SOCK)) > 0){ // Don't need to check SOCKERR_BUSY because it doesn't not occur.
						printf("ok\r\n");
						memset(dbuf, 0, _MAX_SS);
						if(size > _MAX_SS) size = _MAX_SS - 1;
						ret = recv(DATA_SOCK,dbuf,size);
						dbuf[ret] = '\0';
						if(ret != size){
							if(ret==SOCK_BUSY) return 0;
							if(ret < 0){
								printf("%d:recv() error:%ld\r\n",CTRL_SOCK,ret);
								close(DATA_SOCK);
								return ret;
							}
						}
						printf("Rcvd Data:\n\r%s\n\r", dbuf);
						gDataPutGetStart = 0;
						Command.Second = s_nocmd;
					}
					break;
				}
   			}


   			///////////////////////////////


			if((ret=disconnect(DATA_SOCK)) != SOCK_OK) return ret;
			printf("%d:Closed\r\n",DATA_SOCK);
   			break;
   			*/
   		case SOCK_CLOSED :
   			if(ftpc.dsock_state == DATASOCK_READY){
   				if(ftpc.dsock_mode == PASSIVE_MODE){
   					printf("%d:FTP DataStart, port : %d\r\n",DATA_SOCK, local_port);
   					if((ret=socket(DATA_SOCK, Sn_MR_TCP, local_port, 0x0)) != DATA_SOCK){
   						printf("%d:socket() error:%ld\r\n", DATA_SOCK, ret);
   						close(DATA_SOCK);
   						return ret;
   					}
   					local_port++;
   					if(local_port > 50000)
   						local_port = 35000;
   				}else{
   					//local_port++;
   					printf("%d:FTP DataStart, port : %d\r\n",DATA_SOCK, local_port);
   					if((ret=socket(DATA_SOCK, Sn_MR_TCP, local_port, 0x0)) != DATA_SOCK){
   						printf("%d:socket() error:%ld\r\n", DATA_SOCK, ret);
   						close(DATA_SOCK);
   						return ret;
   					}
   					local_port++;
   					if(local_port > 50000)
   						local_port = 35000;
   				}
   				ftpc.dsock_state = DATASOCK_START;
   			}
   			break;

   		case SOCK_INIT :
   			printf("%d:Opened\r\n",DATA_SOCK);
   			if(ftpc.dsock_mode == ACTIVE_MODE){
   				if( (ret = listen(DATA_SOCK)) != SOCK_OK){
   					printf("%d:Listen error\r\n",DATA_SOCK);
   					return ret;
   				}
   				gDataSockReady = 1;
   				printf("%d:Listen ok  data_sock=0x%02x \r\n",DATA_SOCK,getSn_SR(DATA_SOCK));

   			}else{
   				if((ret = connect(DATA_SOCK, remote_ip.cVal, remote_port)) != SOCK_OK){
   					printf("%d:Connect error\r\n", DATA_SOCK);
   					return ret;
   				}
   				gDataSockReady = 1;
   			}
   			connect_state_data_ftpc = 0;
   			break;
   		default :
   			break;
    }
#endif
    return 0;
}

char proc_ftpc(char * buf)
{
	uint16_t Responses;
	uint8_t dat[30]={0,};
	uint8_t pass[30]={0,};

	Responses =(buf[0]-'0')*100+(buf[1]-'0')*10+(buf[2]-'0');
	//printf("%d:proc_ftpc Responses=%d\r\n", CTRL_SOCK,Responses);
	switch(Responses){
		case R_220:	/* Service ready for new user. */

			printf("\r\nInput your User ID > ");
			fflush(stdout);
//			sprintf(dat,"USER %s\r\n", User_Keyboard_MSG());
			sprintf(dat,"USER test\r\n");
			//printf("passwd\r\n");

			//sprintf(pass,"PASS %s\r\n", User_Keyboard_MSG());

			//printf("\r\nSend data = %s\r\n",dat);
			//printf("Send data2 = %s\r\n",pass);
			printf("\r\n");
			send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
			//send(CTRL_SOCK, (uint8_t *)pass, strlen(pass));

			break;

		case R_331:	/* User name okay, need password. */
			printf("\r\nInput your Password > ");
			fflush(stdout);
			//sprintf(dat,"PASS %s\r\n", User_Keyboard_MSG());
			sprintf(dat,"PASS test\r\n");
			printf("\r\n");
			send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
			break;
		case R_230:	/* User logged in, proceed */
			printf("\r\nUser logged in, proceed\r\n");

			sprintf(dat,"TYPE %c\r\n", TransferAscii);
			ftpc.type = ASCII_TYPE;
			send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
			break;

		case 451:	/* User logged in, proceed */
			printf("\r\ninput Port number\r\n");
			sprintf(dat,"PORT %s\r\n", User_Keyboard_MSG());
			printf("\r\n");
			send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
			break;
		case R_200:
			if((ftpc.dsock_mode==ACTIVE_MODE)&&gModeActivePassiveflag){
				ftpc.dsock_state = DATASOCK_READY;
#ifdef	_FTPC_DEBUG_
				printf("\r\nDataSock Ready\r\n");
#endif
				gModeActivePassiveflag = 0;
			}
			else{
				gMenuStart = 1;
			}
			break;
		case R_150:
			switch(Command.First){
			case f_dir:
				printf("\r\nR_150 Get Remote Dir List!!!  D_sock status=0x%02x \r\n",getSn_SR(DATA_SOCK));
				Command.First = f_nocmd;
				Command.Second = s_dir;
				gDataPutGetStart = 1;
				break;
			case f_get:
				printf("\r\nR_150 Get Remote File\r\n");
				Command.First = f_nocmd;
				Command.Second = s_get;
				gDataPutGetStart = 1;
				break;
			case f_put:
				printf("\r\nR_150 Put Remote File\r\n");
				Command.First = f_nocmd;
				Command.Second = s_put;
				gDataPutGetStart = 1;
				break;
			case f_get_files:
				//printf("\r\nR_150 Get Remote Files\r\n");
				Command.First = f_nocmd;
				Command.Second = s_get;
				gDataPutGetStart = 1;
				break;
			default :
				printf("Command.First = default\r\n");
				break;
			}
			break;
		case R_226:
			gMenuStart = 1;
			break;
		case R_227:
			if (pportc(buf) == -1){
				printf("Bad port syntax\r\n");
			}
			else{
				printf("Go Open Data Sock...\r\n ");
				ftpc.dsock_mode = PASSIVE_MODE;
				ftpc.dsock_state = DATASOCK_READY;
			}
			break;
		default:
			printf("\r\nDefault Status = %d\r\n",(uint16_t)Responses);
			buf[0] = 0; buf[1] = 0; buf[2] = 0;
			gDataSockReady = 1;
			break;
		}
	return 1;
}
int pportc(char * arg)
{
	int i;
	char* tok=0;
	strtok(arg,"(");
	for (i = 0; i < 4; i++)
	{
		if(i==0) tok = strtok(NULL,",\r\n");
		else	 tok = strtok(NULL,",");
		remote_ip.cVal[i] = (uint8_t)atoi(tok, 10);
		if (!tok){
			printf("bad pport : %s\r\n", arg);
			return -1;
		}
	}
	remote_port = 0;
	for (i = 0; i < 2; i++){
		tok = strtok(NULL,",\r\n");
		remote_port <<= 8;
		remote_port += atoi(tok, 10);
		if (!tok){
			printf("bad pport : %s\r\n", arg);
			return -1;
		}
	}
	printf("ip : %d.%d.%d.%d, port : %d\r\n", remote_ip.cVal[0], remote_ip.cVal[1], remote_ip.cVal[2], remote_ip.cVal[3], remote_port);
	return 0;
}
uint8_t* User_Keyboard_MSG()
{
	uint8_t i=0;
	do{
		gMsgBuf[i] = ftp_getc();
		i++;
	}while(gMsgBuf[i-1]!=0x0d);
	gMsgBuf[i-1]=0;
	return gMsgBuf;
}


uint8_t Board_UARTGetCharBlocking()
{
	uint8_t rx_data;

	while(1)
	{

		rx_data = 0;
		HAL_UART_Receive(&huart3, &rx_data, 1, 100);
		//printf("Board_UARTGetCharBlocking rx_data = %02x \r\n",rx_data);
	    if(rx_data != 0)  break;
	}
	return rx_data;
}
