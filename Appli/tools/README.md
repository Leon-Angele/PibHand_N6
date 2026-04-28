Run the helper to patch CubeMX-generated main.c so it calls the C++ hook `user_app_main()`.

Usage:

```powershell
python .\Appli\tools\insert_user_hook.py
```

Run this after regenerating code in CubeMX. The script backs up the original `main.c` to `main.c.bak`.
