"""Insert a call to user_app_main() into generated main.c's USER CODE BEGIN 2 block.

Usage: run this script after CubeMX regenerates sources.
It will search for Appli/Core/Src/main.c and, if it finds the USER CODE BEGIN 2 marker,
insert an extern declaration and a call to `user_app_main()` if not already present.
"""
import pathlib

proj_root = pathlib.Path(__file__).resolve().parents[1]
main_c = proj_root / "Core" / "Src" / "main.c"
if not main_c.exists():
    print(f"No main.c found at {main_c}")
    raise SystemExit(1)

text = main_c.read_text(encoding='utf-8')
if 'user_app_main' in text:
    print('user_app_main already referenced in main.c; no change.')
    raise SystemExit(0)

begin = '/* USER CODE BEGIN 2 */'
end = '/* USER CODE END 2 */'
if begin in text and end in text:
    parts = text.split(begin)
    before = parts[0]
    rest = begin.join(parts[1:])
    parts2 = rest.split(end)
    inside = parts2[0]
    after = end.join(parts2[1:])
    inject = '\n/* C++ user hook */\nextern void user_app_main(void);\nuser_app_main();\n'
    new_inside = inside
    # Insert right after the begin marker
    if inject.strip() not in inside:
        new_inside = '\n' + inject + inside
        new_text = before + begin + new_inside + end + after
        backup = main_c.with_suffix('.c.bak')
        backup.write_text(text, encoding='utf-8')
        main_c.write_text(new_text, encoding='utf-8')
        print(f'Patched {main_c}; backup written to {backup}')
    else:
        print('Injection already present inside USER CODE BEGIN 2 block.')
else:
    print('USER CODE BEGIN/END 2 markers not found in main.c; aborting.')
    raise SystemExit(2)
