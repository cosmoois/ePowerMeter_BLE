#pragma once

void ble_init();
bool ble_connectToServer();
bool ble_connectedchk();
void ble_write(String command);
bool ble_promptchk();
