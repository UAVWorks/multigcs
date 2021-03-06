
#include <all.h>

SDL_Thread *gcs_sdl_thread_serial_gps = NULL;
int gcs_serial_fd_gps = -1;
uint8_t gcs_gps_running = 0;
static uint32_t last_connection = 1;
static uint32_t gcs_last_connection = 1;

uint8_t gcsgps_connection_status(void) {
	if (gcs_serial_fd_gps == -1) {
		return 0;
	}
	return gcs_last_connection;
}

uint8_t gps_connection_status(uint8_t modelid) {
	if (ModelData[modelid].serial_fd == -1) {
		return 0;
	}
	return last_connection;
}

void gps_update(uint8_t modelid) {
	static uint8_t n = 0;
	static int res;
	static char serial_buf[2];
	static char line[1024];
	if (ModelData[modelid].serial_fd >= 0 && (res = serial_read(ModelData[modelid].serial_fd, serial_buf, 1)) > 0) {
		last_connection = time(0);
		if (serial_buf[0] == '\r') {
		} else if (serial_buf[0] != '\n') {
			line[n++] = serial_buf[0];
			line[n] = 0;
		} else {
			//			SDL_Log("gps: %s\n", line);
			if (strncmp(line, "$GPGSA", 6) == 0) {
				char autosel;
				int quality;
				sscanf(line, "$GPGSA,%c,%i,", &autosel, &quality);
				GroundData.gpsfix = quality;
				/*
								if (quality == 0) {
									SDL_Log("GPS:  Invalid\n");
								} else if (quality == 1) {
									SDL_Log("GPS:  no Fix\n");
								} else if (quality == 2) {
									SDL_Log("GPS:  2D Fix\n");
								} else if (quality == 3) {
									SDL_Log("GPS:  3D Fix\n");
								}
				*/
			} else if (strncmp(line, "$GPVTG", 6) == 0) {
				float track;
				char ch_T;
				char null1;
				char null2;
				float speed_knots;
				char knots;
				float speed;
				char ch_K;
				sscanf(line, "$GPVTG,%f,%c,%c,%c,%f,%c,%f,%c,", &track, &ch_T, &null1, &null2, &speed_knots, &knots, &speed, &ch_K);
				//				SDL_Log("gps: %s\n", line);
				ModelData[modelid].speed = speed;
				redraw_flag = 1;
			} else if (strncmp(line, "$GNTXT", 6) == 0) {
				SDL_Log("gps: msg: ### %s ###\n", line + 7);
				sys_message(line + 7);
			} else if (strncmp(line, "$GPRMC", 6) == 0) {
				float time;
				char ch_A;
				float lat1;
				char ch_N;
				float lon1;
				char ch_W;
				float speed_knots;
				float curse;
				float date;
				float mag_var;
				char ch_E;
				sscanf(line, "$GPRMC,%f,%c,%f,%c,%f,%c,%f,%f,%f,%f,%c", &time, &ch_A, &lat1, &ch_N, &lon1, &ch_W, &speed_knots, &curse, &date, &mag_var, &ch_E);
				//					SDL_Log("gps: %s %f\n", line, curse);
				ModelData[modelid].yaw = curse;
				redraw_flag = 1;
			} else if (strncmp(line, "$GPGGA", 6) == 0 || strncmp(line, "$GNGGA", 6) == 0) {
				float time;
				float lat1;
				char latdir;
				float lon1;
				char londir;
				int quality;
				int num_sat;
				float hdilution;
				float alt2;
				char alt2_unit;
				float alt1;
				char alt1_unit;
				if (strncmp(line, "$GPGGA", 6) == 0) {
					sscanf(line, "$GPGGA,%f,%f,%c,%f,%c,%d,%d,%f,%f,%c,%f,%c", &time, &lat1, &latdir, &lon1, &londir, &quality, &num_sat, &hdilution, &alt2, &alt2_unit, &alt1, &alt1_unit);
				} else {
					sscanf(line, "$GNGGA,%f,%f,%c,%f,%c,%d,%d,%f,%f,%c,%f,%c", &time, &lat1, &latdir, &lon1, &londir, &quality, &num_sat, &hdilution, &alt2, &alt2_unit, &alt1, &alt1_unit);
				}
				//				SDL_Log("gps: %s\n", line);
				//				SDL_Log("gps: ###################### %f, %f, %c, %f, %c, %d, %d, %f, %f, %c, %f, %c ##\n", time, lat1, latdir, lon1, londir, quality, num_sat, hdilution, alt2, alt2_unit, alt1, alt1_unit);
				char tmp_str[20];
				sprintf(tmp_str, "%2.0f", lat1);
				float hlat = atof(tmp_str + 2) / 60.0;
				tmp_str[2] = 0;
				hlat += atof(tmp_str);
				sprintf(tmp_str, "%5.0f", lon1);
				float hlon = atof(tmp_str + 3) / 60.0;
				tmp_str[3] = 0;
				hlon += atof(tmp_str);
				ModelData[modelid].heartbeat = 100;
				if (hlat != 0.0 && hlon != 0.0) {
					ModelData[modelid].p_lat = hlat;
					ModelData[modelid].p_long = hlon;
					ModelData[modelid].p_alt = alt2;
					ModelData[modelid].numSat = num_sat;
					redraw_flag = 1;
				}
				/*
								SDL_Log("#%f - %f (%0.1fm)#\n", hlat, hlon, alt1);
								SDL_Log("Lat:  %f\n", WayPoints[modelid][0].p_lat);
								SDL_Log("Lon:  %f\n", hlon);
								SDL_Log("Alt:  %0.1fm (%0.1fm)\n", alt1, alt2);
								SDL_Log("Sats: %i\n", num_sat);
				*/
			} else if (strncmp(line, "$IMU", 4) == 0) {
				float imuX;
				float imuY;
				float imuZ;
				sscanf(line, "$IMU,%f,%f,%f", &imuX, &imuY, &imuZ);
				ModelData[modelid].pitch = imuX;
				ModelData[modelid].roll = imuY;
				ModelData[modelid].yaw = imuZ;
				redraw_flag = 1;
			} else if (strncmp(line, "$ACC", 4) == 0) {
				float accX;
				float accY;
				sscanf(line, "$ACC,%f,%f", &accX, &accY);
				ModelData[modelid].acc_x = accX / 90.0;
				ModelData[modelid].acc_y = accY / 90.0;
				redraw_flag = 1;
			} else if (strncmp(line, "$GYRO", 5) == 0) {
				float gyroX;
				float gyroY;
				float gyroZ;
				sscanf(line, "$GYRO,%f,%f,%f", &gyroX, &gyroY, &gyroZ);
				ModelData[modelid].gyro_x = gyroX;
				ModelData[modelid].gyro_y = gyroY;
				ModelData[modelid].gyro_z = gyroZ;
				redraw_flag = 1;
			}
			n = 0;
		}
	}
}


int gcs_thread_serial_gps(void *unused) {
	uint8_t n = 0;
	int res;
	char serial_buf[2];
	char line[1024];
	while (gcs_gps_running == 1) {
		while ((res = serial_read(gcs_serial_fd_gps, serial_buf, 1)) > 0 && gcs_gps_running == 1) {
			gcs_last_connection = time(0);
			if (serial_buf[0] == '\r') {
			} else if (serial_buf[0] != '\n') {
				line[n++] = serial_buf[0];
				line[n] = 0;
			} else {
				if (strncmp(line, "$GPGSA", 6) == 0) {
					char autosel;
					int quality;
					sscanf(line, "$GPGSA,%c,%i,", &autosel, &quality);
					GroundData.gpsfix = quality;
					/*
										if (quality == 0) {
											SDL_Log("GPS:  Invalid\n");
										} else if (quality == 1) {
											SDL_Log("GPS:  no Fix\n");
										} else if (quality == 2) {
											SDL_Log("GPS:  2D Fix\n");
										} else if (quality == 3) {
											SDL_Log("GPS:  3D Fix\n");
										}
					*/
				} else if (strncmp(line, "$GPGGA", 6) == 0 || strncmp(line, "$GNGGA", 6) == 0) {
					float time;
					float lat1;
					char latdir;
					float lon1;
					char londir;
					int quality;
					int num_sat;
					float hdilution;
					float alt2;
					char alt2_unit;
					float alt1;
					char alt1_unit;
					if (strncmp(line, "$GPGGA", 6) == 0) {
						sscanf(line, "$GPGGA,%f,%f,%c,%f,%c,%d,%d,%f,%f,%c,%f,%c", &time, &lat1, &latdir, &lon1, &londir, &quality, &num_sat, &hdilution, &alt2, &alt2_unit, &alt1, &alt1_unit);
					} else {
						sscanf(line, "$GNGGA,%f,%f,%c,%f,%c,%d,%d,%f,%f,%c,%f,%c", &time, &lat1, &latdir, &lon1, &londir, &quality, &num_sat, &hdilution, &alt2, &alt2_unit, &alt1, &alt1_unit);
					}
					//					SDL_Log("%s\n", line);
					//					SDL_Log("###################### %f, %f, %c, %f, %c, %d, %d, %f, %f, %c, %f, %c ##\n", time, lat1, latdir, lon1, londir, quality, num_sat, hdilution, alt2, alt2_unit, alt1, alt1_unit);
					char tmp_str[20];
					sprintf(tmp_str, "%2.0f", lat1);
					float hlat = atof(tmp_str + 2) / 60.0;
					tmp_str[2] = 0;
					hlat += atof(tmp_str);
					sprintf(tmp_str, "%5.0f", lon1);
					float hlon = atof(tmp_str + 3) / 60.0;
					tmp_str[3] = 0;
					hlon += atof(tmp_str);
					if (hlat != 0.0 || hlon != 0.0) {
						int16_t nz = get_altitude(hlat, hlon);
						if ((int16_t)alt2 > nz) {
							nz = (int16_t)alt2;
						}
						//						SDL_Log("%f %f (%i) %i\n", hlat, hlon, nz, (int16_t)alt2);
						GroundData.p_lat = hlat;
						GroundData.p_long = hlon;
						GroundData.p_alt = (float)nz;
						GroundData.numSat = num_sat;
						GroundData.dir = 0.0;
						if (GroundData.active == 0) {
							SDL_Log("ground: found gps (%f %f %i)\n", hlat, hlon, (int16_t)alt2);
						}
						GroundData.active = 1;
						redraw_flag = 1;
					}
					/*
										SDL_Log("#%f - %f (%0.1fm)#\n", hlat, hlon, alt1);
										SDL_Log("Lat:  %f\n", hlat);
										SDL_Log("Lon:  %f\n", hlon);
										SDL_Log("Alt:  %0.1fm (%0.1fm)\n", alt1, alt2);
										SDL_Log("Sats: %i\n", num_sat);
					*/
				}
				n = 0;
			}
		}
		usleep(100000);
	}
	SDL_Log("gcs-gps: exit gps\n");
	return 0;
}


uint8_t gcs_gps_init(char *port, uint32_t baud) {
	SDL_Log("gcs-gps: init serial port...\n");
	gcs_serial_fd_gps = serial_open(port, baud);
	gcs_gps_running = 1;
	if (gcs_serial_fd_gps != -1) {
#ifdef SDL2
		gcs_sdl_thread_serial_gps = SDL_CreateThread(gcs_thread_serial_gps, NULL, NULL);
#else
		gcs_sdl_thread_serial_gps = SDL_CreateThread(gcs_thread_serial_gps, NULL);
#endif
		if (gcs_sdl_thread_serial_gps == NULL) {
			fprintf(stderr, "gcs-gps: Unable to create gcs_thread_serial_gps: %s\n", SDL_GetError());
			return 1;
		}
	}
	return 0;
}

uint8_t gps_init(uint8_t modelid, char *port, uint32_t baud) {
	SDL_Log("gps(%i): init serial port...\n", modelid);
	ModelData[modelid].serial_fd = serial_open(port, baud);
	return 0;
}

void gcs_gps_exit(void) {
	gcs_gps_running = 0;
	if (gcs_sdl_thread_serial_gps != NULL) {
		SDL_Log("gcs-gps: wait thread\n");
		SDL_WaitThread(gcs_sdl_thread_serial_gps, NULL);
		gcs_sdl_thread_serial_gps = NULL;
	}
	serial_close(gcs_serial_fd_gps);
	gcs_serial_fd_gps = -1;
}

void gps_exit(uint8_t modelid) {
	if (ModelData[modelid].serial_fd >= 0) {
		serial_close(ModelData[modelid].serial_fd);
		ModelData[modelid].serial_fd = -1;
	}
}



