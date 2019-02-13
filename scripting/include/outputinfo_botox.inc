#if defined _OutputInfo_Botox_Included
 #endinput
#endif
#define _OutputInfo_Botox_Included

#include <outputinfo>

stock int FindOutput(int entity, const char[] output, int startindex, const char[] target = NULL_STRING, const char[] targetinput = NULL_STRING, const char[] parameter = NULL_STRING, float delay = -1.0, int timestofire = 0)
{
	char buffer[256];
	int count = GetOutputActionCount(entity, output);
	for (int i = startindex; i < count; i++) {
		if (target[0] != '\0') {
			if (!GetOutputActionTarget(entity, output, i, buffer, sizeof(buffer))) {
				continue;
			}
			if (strncmp(buffer, target, sizeof(buffer)) != 0) {
				continue;
			}
		}
		if (targetinput[0] != '\0') {
			if (!GetOutputActionTargetInput(entity, output, i, buffer, sizeof(buffer))) {
				continue;
			}
			if (strncmp(buffer, targetinput, sizeof(buffer)) != 0) {
				continue;
			}
		}
		if (parameter[0] != '\0') {
			if (!GetOutputActionParameter(entity, output, i, buffer, sizeof(buffer))) {
				continue;
			}
			if (strncmp(buffer, targetinput, sizeof(buffer)) != 0) {
				continue;
			}
		}
		if (delay != -1.0) {
			float actiondelay = GetOutputActionDelay(entity, output, i);
			if (actiondelay == -1.0) {
				continue;
			}
			if (actiondelay != delay) {
				continue;
			}
		}
		if (timestofire != 0) {
			if (timestofire != GetOutputActionTimesToFire(entity, output, i)) {
				continue;
			}
		}
		return i;
	}
	return -1;
}

stock int GetOutputCount(int entity, const char[] output) {
	return GetOutputActionCount(entity, output);
}

stock int GetOutputTarget(int entity, const char[] output, int index, char[] target, int maxlen) {
	if (!GetOutputActionTarget(entity, output, index, target, maxlen)) {
		return 0;
	}
	return strlen(target);
}

stock int GetOutputTargetInput(int entity, const char[] output, int index, char[] targetinput, int maxlen) {
	if (!GetOutputActionTargetInput(entity, output, index, targetinput, maxlen)) {
		return 0;
	}
	return strlen(targetinput);
}

stock int GetOutputParameter(int entity, const char[] output, int index, char[] parameter, int maxlen) {
	if (!GetOutputActionParameter(entity, output, index, parameter, maxlen)) {
		return 0;
	}
	return strlen(parameter);
}

stock float GetOutputDelay(int entity, const char[] output, int index)
{
	return GetOutputActionDelay(entity, output, index);
}

stock int GetOutputFormatted(int entity, const char[] output, int index, char[] formatted, int maxlen) {
	char target[256];
	char targetinput[256];
	char parameter[256];
	if (!GetOutputTarget(entity, output, index, target, sizeof(target))) {
		return 0;
	}
	if (!GetOutputTargetInput(entity, output, index, targetinput, sizeof(targetinput))) {
		return 0;
	}
	if (!GetOutputParameter(entity, output, index, parameter, sizeof(parameter))) {
		return 0;
	}
	float delay = GetOutputDelay(entity, output, index);
	int timestofire = GetOutputActionTimesToFire(entity, output, index);
	return FormatEx(formatted, maxlen, "%s,%s,%s,%f,%d", target, targetinput, parameter, delay, timestofire);
}

stock bool DeleteOutput(int entity, const char[] output, int index) {
	return RemoveOutputAction(entity, output, index);
}

stock int DeleteAllOutputs(int entity, const char[] output)
{
	int count = GetOutputCount(entity, output);
	for (int i = count - 1; i >= 0; count--) {
		DeleteOutput(entity, output, i);
	}
	return count;
}