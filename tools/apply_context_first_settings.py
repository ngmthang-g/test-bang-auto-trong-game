from pathlib import Path

p = Path('src/bridge.cpp')
s = p.read_text(encoding='utf-8-sig')
old = '''    auto selection = auto_menu_ui_logic::SelectSettingsChoice(candidates);\n    if (selection.kind == auto_menu_ui_logic::SelectionKind::Ambiguous) {\n        // Only duplicate exact-label candidates pay the RectTransform/Screen cost.\n'''
new = '''    auto selection = auto_menu_ui_logic::SelectSettingsChoiceContext(candidates);\n    if (selection.kind == auto_menu_ui_logic::SelectionKind::Ambiguous) {\n        // Ancestor context is the cheapest proof. Only if that is still\n        // ambiguous do duplicate exact-label candidates pay the\n        // RectTransform/Screen fallback cost.\n'''
if s.count(old) != 1:
    raise SystemExit(f'expected one selection anchor, got {s.count(old)}')
s = s.replace(old, new, 1)
s = s.replace("nhiều 'Thiết lập'; upper-region tie-break chưa UNIQUE",
              "nhiều 'Thiết lập'; AutoFightGroup/TopIcon context + position fallback chưa UNIQUE")
p.write_text(s, encoding='utf-8-sig')
