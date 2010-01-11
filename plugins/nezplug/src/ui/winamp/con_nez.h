#ifndef CON_NEZ_H_
#define CON_NEZ_H_

#ifdef __cplusplus
extern "C" {
#endif

int GetControler(void);
int GetRestart(void);
void SetStateControler(int state);

void InstallControler(int bControlerEnable);
void UninstallControler(void);
void EnableControler(int starts, int nums);
void DisableControler(void);

void SubclassWinamp(void);
void UnsubclassWinamp(void);

#ifdef __cplusplus
}
#endif

#endif
