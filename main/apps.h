void initapps();

#define appscount 3

//extern const int appscount;
extern void (*uapp[])();
extern void (*iapp[])();
extern void (*dapp[])();

void initbars();
void barsupdate();
