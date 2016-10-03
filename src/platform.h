#ifdef __cplusplus

extern "C" void Platform_CreateWindow(int x, int y, int width, int height, int useDoubleBuffer);
extern "C" void Platform_PollEvents(void);
extern "C" void Platform_Sleep(int ms);

#endif
