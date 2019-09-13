
’
OVTSProto.proto"7
custom_struct
key (	Rkey
value (	Rvalue"|
device_timer_struct!
network_send (RnetworkSend'
network_receive (RnetworkReceive
sms_send (RsmsSend"µ
device_data_select_struct

gps_enable (R	gpsEnable

imu_enable (R	imuEnable!
relay_enable (RrelayEnable.
input_sensor_enable (RinputSensorEnable,
sim_balance_enable (RsimBalanceEnable%
battery_enable (RbatteryEnable2
signal_quality_enable (RsignalQualityEnable"¨
device_params_struct*
timer (2.device_timer_structRtimer;
data_select (2.device_data_select_structR
dataSelect'
connection_type (RconnectionType"¶
center_address_struct&
device_data_url (	RdeviceDataUrl.
center_commands_url (	RcenterCommandsUrl*
device_report_url (	RdeviceReportUrl
ping_url (	RpingUrl"£
center_params_struct=
center_address (2.center_address_structRcenterAddress
number1 (	Rnumber1
number2 (	Rnumber2
number3 (	Rnumber3"š

DeviceData
	device_id (RdeviceId
time (Rtime6
gps_data (2.DeviceData.gps_data_structRgpsData6
imu_data (2.DeviceData.imu_data_structRimuData
relay (Rrelay!
input_sensor (RinputSensor
sim_balance (R
simBalance=
battery_data (2.DeviceData.battery_structRbatteryData%
signal_quality	 (RsignalQuality:
center_params
 (2.center_params_structRcenterParams:
device_params (2.device_params_structRdeviceParams1
custom_field (2.custom_structRcustomField¡
imu_data_struct
ax (Rax
ay (Ray
az (Raz
gx (Rgx
gy (Rgy
gz (Rgz
mx (Rmx
my (Rmy
mz	 (Rmz“
gps_data_struct
lat (Rlat
lng (Rlng
speed (Rspeed
altitude (Raltitude
course (Rcourse
hdop (RhdopF
battery_struct
capacity (Rcapacity
plugged (Rplugged"è
CenterCommands
	device_id (RdeviceIdT
set_center_params (2(.CenterCommands.set_center_params_structRsetCenterParams:
device_params (2.device_params_structRdeviceParamsB
device_report (2.CenterCommands.report_structRdeviceReport?
device_sleep (2.CenterCommands.sleep_structRdeviceSleep
relay (Rrelay1
custom_field (2.custom_structRcustomField7
report_struct
start (Rstart
end (Rend6
sleep_struct
start (Rstart
end (Rendh
set_center_params_struct
key (	Rkey:
center_params (2.center_params_structRcenterParams