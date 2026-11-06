"""Build Part 3 project schedule sheet into the Part 2 workbook."""

from datetime import datetime, timedelta
from openpyxl import load_workbook
from openpyxl.styles import Font, PatternFill, Alignment, Border, Side
from openpyxl.utils import get_column_letter

SRC = r"c:\Users\jamdl\Downloads\ProjectSchedule_Part2_GADE7322.xlsx"
OUT_COPY = r"c:\Users\jamdl\Downloads\ProjectSchedule_Part3_GADE7322.xlsx"


def fill(hex_):
    return PatternFill("solid", fgColor=hex_)


def main():
    wb = load_workbook(SRC)
    ws2 = wb["Part 2 Schedule"]

    if "Part 3 Schedule" in wb.sheetnames:
        del wb["Part 3 Schedule"]

    ws = wb.create_sheet("Part 3 Schedule")

    thin = Border(
        left=Side(style="thin", color="FFD5D8DC"),
        right=Side(style="thin", color="FFD5D8DC"),
        top=Side(style="thin", color="FFD5D8DC"),
        bottom=Side(style="thin", color="FFD5D8DC"),
    )
    font_w = Font(bold=True, color="FFFFFFFF", name="Calibri", size=11)
    font_sec = Font(bold=True, color="FF4A235A", name="Calibri", size=11)
    font_n = Font(bold=False, color="FF000000", name="Calibri", size=10)

    C_TITLE = "FF4A235A"
    C_DELIV = "FF5B2C6F"
    C_SEC_BG = "FFF4ECF7"
    C_TIYAH = "FF5DADE2"
    C_TIYAH_ROW = "FFD6EAF8"
    C_JAMIE = "FFAF7AC5"
    C_JAMIE_ROW = "FFE8DAEF"
    C_SHARED = "FF58D68D"
    C_SHARED_ROW = "FFD5F5E3"
    C_INDIV = "FFF5B041"
    C_INDIV_ROW = "FFFDEBD0"
    C_WEEKEND = "FFF5B7B1"
    C_DATE = "FFD7BDE2"
    C_DEADLINE = "FFE74C3C"
    C_SECTION_GANTT = "FFF5EEF8"
    C_SETBACK = "FFFADBD8"
    C_SETBACK_HDR = "FFFF6B6B"
    P0, P25, P50, P75, P100 = "FFE8D3FD", "FFD7B3FB", "FFC794FA", "FFA95AF8", "FF7C0BEF"

    for col in range(1, 34):
        letter = get_column_letter(col)
        ws.column_dimensions[letter].width = ws2.column_dimensions[letter].width

    ws.row_dimensions[2].height = 18
    ws.row_dimensions[3].height = 30

    start = datetime(2026, 10, 20)
    dates = [start + timedelta(days=i) for i in range(22)]

    def is_weekend(d):
        return d.weekday() >= 5

    ws["L1"] = "Date"
    ws["L1"].font = Font(bold=True, color="FF4A235A")
    ws.merge_cells("L1:AG2")
    ws["L1"].alignment = Alignment(horizontal="center", vertical="center")

    ws["A2"] = "GADE POE PART 3"
    ws["B2"] = "Member Allocated"
    ws["C2"] = "Roles & Responsibilities"
    ws["D2"] = "Role Tasks"
    ws["E2"] = "Asset Tracker"
    ws.merge_cells("A2:A3")
    ws.merge_cells("B2:B3")
    ws.merge_cells("C2:C3")
    ws.merge_cells("D2:D3")
    for col in range(1, 5):
        cell = ws.cell(2, col)
        cell.fill = fill(C_TITLE)
        cell.font = font_w
        cell.alignment = Alignment(horizontal="center", vertical="center", wrap_text=True)

    headers = [
        (5, "Begin 0% Complete", P0),
        (6, "Started  25% Complete", P25),
        (7, "Halfway 50% Complete", P50),
        (8, "Finishing 75% Complete", P75),
        (9, "Done 100% Complete", P100),
    ]
    for col, text, hex_ in headers:
        c = ws.cell(3, col, text)
        c.fill = fill(hex_)
        c.font = Font(bold=True, color="FF4A235A", size=9)
        c.alignment = Alignment(wrap_text=True, horizontal="center", vertical="center")

    ws["J3"] = "Setbacks Faced"
    ws["J3"].fill = fill(C_SETBACK_HDR)
    ws["J3"].font = Font(bold=True, color="FFFFFFFF", size=9)
    ws["J3"].alignment = Alignment(wrap_text=True, horizontal="center", vertical="center")
    ws["K3"] = "Outline of the Problem"
    ws["K3"].font = Font(bold=True, size=9)
    ws["K3"].alignment = Alignment(wrap_text=True, horizontal="center", vertical="center")

    for i, d in enumerate(dates):
        col = 12 + i
        cell = ws.cell(3, col, d)
        cell.number_format = "DD/MM/YYYY"
        cell.alignment = Alignment(horizontal="center", textRotation=90)
        if i == len(dates) - 1:
            cell.fill = fill(C_DEADLINE)
            cell.font = Font(bold=True, color="FFFFFFFF", size=9)
        elif is_weekend(d):
            cell.fill = fill(C_WEEKEND)
            cell.font = Font(bold=True, color="FF4A235A", size=9)
        else:
            cell.fill = fill(C_DATE)
            cell.font = Font(bold=True, color="FF4A235A", size=9)

    ws["A5"] = "Key"
    ws["B5"] = "Tiyah Singh"
    ws["C5"] = "Jamie-Lee Davies"
    ws["D5"] = "Shared"
    ws["E5"] = "Individual"
    ws["F5"] = "Weekend"
    ws["B5"].fill = fill(C_TIYAH)
    ws["B5"].font = font_w
    ws["C5"].fill = fill(C_JAMIE)
    ws["C5"].font = font_w
    ws["D5"].fill = fill(C_SHARED)
    ws["D5"].font = font_w
    ws["E5"].fill = fill(C_INDIV)
    ws["E5"].font = font_w
    ws["F5"].fill = fill(C_WEEKEND)
    ws["F5"].font = Font(bold=True)
    ws.merge_cells("G5:K5")
    ws["G5"] = "Part 3 — Upgrades, Shaders, VFX & Procedural Features"
    ws["G5"].font = Font(bold=True, italic=True, color="FF4A235A")

    ws["A6"] = "Section"
    ws["A6"].font = font_sec
    ws["A6"].fill = fill(C_SEC_BG)
    for c in range(12, 34):
        ws.cell(6, c).fill = fill(C_SECTION_GANTT)

    def member_colors(member):
        if member == "Tiyah Singh":
            return C_TIYAH, C_TIYAH_ROW
        if member == "Jamie-Lee Davies":
            return C_JAMIE, C_JAMIE_ROW
        if member == "Shared":
            return C_SHARED, C_SHARED_ROW
        return C_INDIV, C_INDIV_ROW

    def progress_colors(n):
        if n <= 1:
            return [P100]
        if n == 2:
            return [P50, P100]
        if n == 3:
            return [P25, P75, P100]
        if n == 4:
            return [P0, P25, P75, P100]
        return ([P0, P25, P50, P75] + [P100] * (n - 4))[:n]

    rows = []
    rows.append(("deliverable", "Deliverable 1"))
    rows.append(("section", "Upgradable Defenders & Tower"))
    rows += [
        ("task", "Jamie-Lee Davies", "Class Upgrade System",
         "Build PurchaseClassUpgrade with gold cost and cost growth", (0, 4), None),
        ("task", None, None, "Support 2 upgrade levels per defender type and tower", (2, 3), None),
        ("task", None, None, "Apply health and damage multipliers on upgrade", (3, 3), None),
        ("task", None, None, "Make upgrades class-wide so new units inherit levels", (4, 3), None),
        ("task", "Tiyah Singh", "Upgrade Visuals",
         "Attach UpgradeAuraComponent so upgrades change appearance", (3, 4), None),
        ("task", None, None, "Shift aura colour by upgrade level (Lv1 vs Lv2)", (5, 3), None),
        ("task", None, None, "Wire tower / princess upgrades into the same flow", (6, 3), None),
        ("task", "Shared", "Balance & Testing",
         "Retune wave pressure so upgrades stay challenging", (7, 4), None),
        ("task", None, None, "Playtest gold economy vs upgrade spend", (8, 3),
         ("Yes", "Upgrade cost felt too cheap early — raised cost growth")),
    ]

    rows.append(("deliverable", "Deliverable 2"))
    rows.append(("section", "Custom Shaders"))
    rows += [
        ("task", "Jamie-Lee Davies", "Shader 1 — Waving Grass",
         "Author M_WavingGrass with World Position Offset vertex displace", (5, 5), None),
        ("task", None, None, "Apply waving grass material to ground / grass tiles", (7, 3), None),
        ("task", None, None, "Tune amplitude / speed so wind reads in-game", (8, 3),
         ("Yes", "First WPO amplitude made tiles flash — rebuilt with stylized UVs")),
        ("task", "Tiyah Singh", "Shader 2 — Upgrade Aura",
         "Author M_UpgradeAura changing base colour (pink / Fresnel wrap)", (6, 5), None),
        ("task", None, None, "Hook aura material into UpgradeAuraComponent MID", (8, 3), None),
        ("task", "Shared", "Shader Review",
         "Confirm one shader displaces verts and one changes base colour", (10, 3), None),
    ]

    rows.append(("deliverable", "Deliverable 3"))
    rows.append(("section", "Custom Visual Effects"))
    rows += [
        ("task", "Jamie-Lee Davies", "Particle VFX",
         "Create custom upgrade spark particle system (PS/NS_UpgradeBurst)", (9, 5), None),
        ("task", None, None, "Add UpgradeSparkBurstActor fallback burst for PIE reliability", (11, 3), None),
        ("task", None, None, "Trigger PlayUpgradeBurstAt from PurchaseClassUpgrade", (12, 3), None),
        ("task", "Tiyah Singh", "Screen-Space VFX",
         "Author M_PP_TowerThreat post-process (edge darken + red tint)", (10, 5), None),
        ("task", None, None,
         "Drive ThreatIntensity from princess tower HP in GameplayVfxSubsystem", (12, 4), None),
        ("task", None, None, "Spawn unbound PostProcessVolume and ramp below ~70% HP", (13, 3), None),
        ("task", "Shared", "VFX Testing",
         "Verify sparks on upgrade and vignette when tower is threatened", (14, 3), None),
    ]

    rows.append(("deliverable", "Deliverable 4"))
    rows.append(("section", "Planning Custom Procedural Feature"))
    rows += [
        ("task", "Individual", "Jamie Planning Doc",
         "Jamie: research seeded elite enemy phenotypes", (0, 4), None),
        ("task", None, None, "Jamie: write 400-500 word elite goblin procedural plan", (2, 4), None),
        ("task", None, None, "Jamie: diagrams for seed → scale/tint/stats/horn flow", (4, 3), None),
        ("task", "Individual", "Tiyah Planning Doc",
         "Tiyah: research procedural wave omen / narrative events", (1, 4), None),
        ("task", None, None, "Tiyah: write 400-500 word omen generation plan", (3, 4), None),
        ("task", None, None, "Tiyah: diagrams for roll table and ApplyOmenToPlan effects", (5, 3), None),
        ("task", "Shared", "Review",
         "Peer review both docs against brief planning questions", (6, 3), None),
    ]

    rows.append(("deliverable", "Deliverable 5"))
    rows.append(("section", "Implementing Procedural Features"))
    rows += [
        ("task", "Jamie-Lee Davies", "Procedural Elite Goblins",
         "Mark elite spawns in wave strategies (AssignEliteSpawns)", (8, 4), None),
        ("task", None, None, "Build ProceduralEliteComponent ApplyPhenotype from seed", (10, 5), None),
        ("task", None, None, "Roll scale, tint, HP/speed/gold, optional horn mesh", (12, 4), None),
        ("task", None, None, "Attach elite component on spawn when bElite is set", (14, 3), None),
        ("task", "Tiyah Singh", "Procedural Wave Omens",
         "Define EWaveOmenType and FWaveOmen in WaveTypes", (9, 3), None),
        ("task", None, None, "Implement RollOmen each intermission with run seed", (11, 4), None),
        ("task", None, None,
         "Implement ApplyOmenToPlan (flood, treasury, berserker moon, etc.)", (13, 5), None),
        ("task", None, None, "Show OMEN on debug HUD / log for verification", (15, 3),
         ("Yes", "Omen text only on debug HUD — toggled for video demos")),
        ("task", "Shared", "Integration & Testing",
         "Confirm elites and omens integrate with Budget and Counter strategies", (16, 4), None),
        ("task", None, None, "Check implementation matches both planning documents", (17, 3), None),
    ]

    rows.append(("deliverable", "Deliverable 6"))
    rows.append(("section", "Polish & Complexity"))
    rows += [
        ("task", "Tiyah Singh", "Gameplay Polish",
         "UI art pass — menus, pause, game over, HUD readability", (12, 5), None),
        ("task", None, None, "Start menu strategy pick (Threat Budget / Counter-Play)", (14, 3), None),
        ("task", "Jamie-Lee Davies", "Systems Cleanup",
         "Hide placement pads when occupied; enlarge tower meshes", (13, 4), None),
        ("task", None, None, "Toggle debug HUD for clean footage vs omen demo", (18, 2), None),
        ("task", "Shared", "Rubric Check",
         "Final check against Part 3 upgrades / shaders / VFX / procedural", (18, 3), None),
    ]

    rows.append(("deliverable", "Individual"))
    rows.append(("section", "Presentation"))
    rows += [
        ("task", "Individual", "Gameplay Demo",
         "Record Part 3 gameplay: upgrades, shaders, VFX, procedural feature", (17, 4), None),
        ("task", None, None, "Jamie: show Shader 1, particle burst, elite goblins", (18, 3), None),
        ("task", "Individual", "Code Explanation",
         "Tiyah: show Shader 2, threat PP, procedural omens", (18, 3), None),
        ("task", None, None, "Keep each video around 5–6 minutes covering Part 3 rubric", (19, 2), None),
    ]

    rows.append(("deliverable", "Overall Submission"))
    rows.append(("section", "General"))
    rows += [
        ("task", "Tiyah Singh", "Referencing",
         "Harvard references in both Part 3 planning documents", (15, 4), None),
        ("task", None, None, "Code referencing and bibliography updates", (17, 3), None),
        ("task", "Jamie-Lee Davies", "Github Maintenance",
         "Keep repo updated with clear Part 3 commits", (10, 8), None),
        ("task", None, None, "Prepare final commit for Arc link", (19, 2), None),
        ("task", "Shared", "Final Submit", "Submit GitHub link to Arc", (20, 2), None),
        ("task", None, None, "Submit both individual planning documents", (20, 2), None),
        ("task", None, None, "Submit individual Part 3 presentation videos", (20, 2), None),
    ]

    def paint_gantt(row, start_idx, length):
        cols = progress_colors(length)
        for i, hex_ in enumerate(cols):
            idx = start_idx + i
            if idx >= 22:
                break
            ws.cell(row, 12 + idx).fill = fill(hex_)
        for i in range(22):
            cell = ws.cell(row, 12 + i)
            rgb = getattr(cell.fill.fgColor, "rgb", None) if cell.fill and cell.fill.fgColor else None
            if rgb in (None, "00000000"):
                cell.fill = fill(C_WEEKEND if is_weekend(dates[i]) else "FFFFFFFF")

    r = 7
    last_member = None
    last_role = None
    member_start = None
    role_start = None

    def flush_member_role(end_row):
        nonlocal member_start, role_start, last_member, last_role
        if last_member and member_start and end_row >= member_start:
            if end_row > member_start:
                ws.merge_cells(start_row=member_start, start_column=2, end_row=end_row, end_column=2)
        if last_role and role_start and end_row >= role_start:
            if end_row > role_start:
                ws.merge_cells(start_row=role_start, start_column=3, end_row=end_row, end_column=3)
        member_start = role_start = None
        last_member = last_role = None

    i_row = 0
    while i_row < len(rows):
        item = rows[i_row]
        kind = item[0]

        if kind == "deliverable":
            flush_member_role(r - 1)
            ws.cell(r, 1, item[1])
            for c in range(1, 12):
                ws.cell(r, c).fill = fill(C_DELIV)
                ws.cell(r, c).font = font_w
            for c in range(12, 34):
                ws.cell(r, c).fill = fill(C_DELIV)
            r += 1
            i_row += 1
            continue

        if kind == "section":
            flush_member_role(r - 1)
            span = 0
            for j in range(i_row + 1, len(rows)):
                if rows[j][0] != "task":
                    break
                span += 1
            ws.cell(r, 1, item[1])
            ws.cell(r, 1).fill = fill(C_SEC_BG)
            ws.cell(r, 1).font = font_sec
            ws.cell(r, 1).alignment = Alignment(vertical="center", wrap_text=True)
            if span > 1:
                ws.merge_cells(start_row=r, start_column=1, end_row=r + span - 1, end_column=1)
                for rr in range(r, r + span):
                    ws.cell(rr, 1).fill = fill(C_SEC_BG)
            i_row += 1
            continue

        if kind == "task":
            member, role, task = item[1], item[2], item[3]
            gantt = item[4]
            setback = item[5]

            m_for_color = member or last_member or "Shared"
            hdr, row_fill_c = member_colors(m_for_color)

            if member:
                flush_member_role(r - 1)
                ws.cell(r, 2, member)
                ws.cell(r, 2).fill = fill(hdr)
                ws.cell(r, 2).font = font_w
                ws.cell(r, 2).alignment = Alignment(
                    vertical="center", wrap_text=True, horizontal="center"
                )
                last_member = member
                member_start = r

            if role:
                ws.cell(r, 3, role)
                ws.cell(r, 3).fill = fill(row_fill_c)
                ws.cell(r, 3).font = Font(bold=True, color="FF1C2833")
                ws.cell(r, 3).alignment = Alignment(vertical="center", wrap_text=True)
                last_role = role
                role_start = r

            ws.cell(r, 4, task)
            ws.cell(r, 4).fill = fill(row_fill_c)
            ws.cell(r, 4).font = font_n
            ws.cell(r, 4).alignment = Alignment(wrap_text=True, vertical="center")

            for c in range(5, 10):
                ws.cell(r, c).fill = fill(row_fill_c)

            if setback:
                ws.cell(r, 10, setback[0])
                ws.cell(r, 11, setback[1])
                ws.cell(r, 10).fill = fill(C_SETBACK)
                ws.cell(r, 11).fill = fill(C_SETBACK)
                ws.cell(r, 10).font = Font(bold=True)
                ws.cell(r, 11).alignment = Alignment(wrap_text=True)
            else:
                ws.cell(r, 10).fill = fill("FFFFFFFF")
                ws.cell(r, 11).fill = fill("FFFFFFFF")

            paint_gantt(r, gantt[0], gantt[1])

            for c in range(1, 34):
                ws.cell(r, c).border = thin

            r += 1
            i_row += 1
            continue

        i_row += 1

    flush_member_role(r - 1)
    ws.freeze_panes = "L4"
    wb.move_sheet(ws, offset=-len(wb.sheetnames) + 1)

    wb.save(SRC)
    wb.save(OUT_COPY)
    print("Updated", SRC)
    print("Also saved", OUT_COPY)
    print("Sheets:", wb.sheetnames)
    print("Last data row:", r - 1)


if __name__ == "__main__":
    main()
