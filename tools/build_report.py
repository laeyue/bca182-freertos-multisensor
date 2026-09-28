"""Generate the academic laboratory report from verified project facts."""

from pathlib import Path
from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.units import inch
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak,
)


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "docs" / "laboratory-report.pdf"
styles = getSampleStyleSheet()
styles.add(ParagraphStyle(name="TitleCustom", parent=styles["Title"], fontSize=17,
                          leading=21, spaceAfter=14, textColor=colors.HexColor("#15344b")))
styles.add(ParagraphStyle(name="SectionCustom", parent=styles["Heading1"], fontSize=12,
                          leading=15, spaceBefore=12, spaceAfter=6,
                          textColor=colors.HexColor("#15344b")))
styles.add(ParagraphStyle(name="BodyCustom", parent=styles["BodyText"], fontSize=9,
                          leading=13, spaceAfter=7))
styles.add(ParagraphStyle(name="SmallCustom", parent=styles["BodyText"], fontSize=7.5,
                          leading=10))
styles.add(ParagraphStyle(name="SubCustom", parent=styles["Heading2"], fontSize=10,
                          leading=12, spaceBefore=8, spaceAfter=4))
styles.add(ParagraphStyle(name="CenteredCustom", parent=styles["BodyText"],
                          alignment=TA_CENTER, fontSize=9, leading=13))

story = []


def p(text, style="BodyCustom"):
    story.append(Paragraph(text, styles[style]))


def section(title):
    p(title, "SectionCustom")


def sub(title):
    p(title, "SubCustom")


def table(headers, rows, widths):
    values = [[Paragraph(str(v), styles["SmallCustom"]) for v in headers]]
    values += [[Paragraph(str(v), styles["SmallCustom"]) for v in row] for row in rows]
    item = Table(values, colWidths=widths, repeatRows=1, hAlign="LEFT")
    item.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#dbe9f1")),
        ("GRID", (0, 0), (-1, -1), 0.35, colors.HexColor("#9fb1bb")),
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("LEFTPADDING", (0, 0), (-1, -1), 5),
        ("RIGHTPADDING", (0, 0), (-1, -1), 5),
        ("TOPPADDING", (0, 0), (-1, -1), 4),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 4),
    ]))
    story.append(item)
    story.append(Spacer(1, 8))


p("BCA182 Laboratory Activity 1", "TitleCustom")
p("Real-Time Multisensor Room Monitoring System", "CenteredCustom")
p("Name: Kent Alexis Alia&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Section: B182&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Date: 9/28/26")
p("Status: firmware build and host unit tests verified; interactive Wokwi verification and fault experiments remain pending.")

section("1. Problem and Requirements")
p("The device monitors temperature, humidity, relative brightness and motion with a simulated STM32F103C8. A rotary encoder selects one of four OLED pages. A PWM buzzer indicates temperatures strictly below 18 C or strictly above 30 C. A 15-second no-motion interval enters INACTIVE; PIR activity restores ACTIVE. The architecture must use multiple FreeRTOS tasks, queues, a mutex, an event mechanism and periodic execution.")
p("The laboratory handout contains a contradictory statement that the configuration must identify ESP-IDF, adjacent to a STM32Cube example. The STM32 Blue Pill target and all surrounding framework requirements specify STM32Cube, so this project uses <b>framework = stm32cube</b>. Wokwi-supported ADC1, I2C1, USART1, EXTI and TIM2/TIM4 features are used; no Arduino abstraction is included.")

section("2. System Architecture and Design")
table(["Subsystem", "Device / pins", "Responsibilities"], [
    ("Sensing", "DHT22 PB12; LDR AO PA0", "DHT pulse decoding and checksum; ADC1 relative brightness"),
    ("Presence / input", "PIR PB13; encoder PA1/PA2", "Motion event and page navigation"),
    ("Output", "SSD1306 PB6/PB7; buzzer PB8", "I2C display and TIM4 500 Hz PWM alarm"),
    ("Diagnostics", "USART1 PA9", "Serialized task log output"),
], [1.05*inch, 1.65*inch, 3.95*inch])
p("StateTask owns the ACTIVE/INACTIVE state. On each PIR-high observation it refreshes lastMotion. When elapsed time since that observation reaches 15,000 ms, it clears EVENT_ACTIVE. A new EVENT_MOTION restores ACTIVE. DisplayTask turns the OLED off and blocks on EVENT_ACTIVE while inactive. AlarmTask mutes the buzzer in that state; sensor sampling continues to keep readings fresh.")

section("3. FreeRTOS Architecture")
table(["Task", "Priority", "Trigger / period", "IPC", "Typical blocked condition"], [
    ("MotionTask", "3", "50 ms", "EVENT_MOTION; motion queue", "DelayUntil"),
    ("InputTask", "3", "Encoder EXTI", "Encoder and page queues", "Queue receive"),
    ("StateTask", "3", "Motion / 100 ms", "Event group", "Event wait"),
    ("SensorTask", "2", "2 s", "Display + alarm queues", "DelayUntil"),
    ("AlarmTask", "2", "Sensor / 100 ms", "Alarm queue; EVENT_ALARM", "Queue receive"),
    ("DisplayTask", "1", "Update / 100 ms", "Sensor, page, motion queues", "Queue or ACTIVE wait"),
], [1.05*inch, .52*inch, 1.05*inch, 2.05*inch, 1.98*inch])
p("Motion, encoder and state transitions have the tightest response budgets and use priority 3. Two-second sensor sampling and alarm evaluation tolerate priority 2. OLED transfers tolerate additional latency and use priority 1. All loops block or wait, avoiding a continuously ready high-priority task. The only short busy interval is the DHT timing transaction inside a critical section.")
p("SensorTask overwrites two separate single-item SensorData queues. DisplayTask and AlarmTask must both see the newest reading; a single destructive-consumption queue would distribute, rather than broadcast, messages. InputTask consumes signed encoder steps from an ISR queue and writes the current DisplayMode to a mailbox queue. MotionTask sends a separate motion boolean to DisplayTask because the PIR and DHT/ADC have distinct task owners.")
p("EVENT_MOTION is set by MotionTask on PIR-high samples and consumed/cleared by StateTask. EVENT_ACTIVE is set or cleared only by StateTask and read by display, input and alarm tasks. EVENT_ALARM is set or cleared only by AlarmTask and read by DisplayTask. These bits represent actual application events and level state, rather than unused placeholders.")
p("The mutex protects USART1 transmission. SensorTask, MotionTask, InputTask, StateTask and AlarmTask may log concurrently. Without serialization, a task switch between bytes could interleave diagnostic lines. The OLED is single-owner and does not require a mutex.")
p("A Running task currently uses the CPU. A Ready task can run but waits for scheduler selection. A Blocked task waits for time, a queue or an event and consumes no CPU. This application deliberately uses Blocked states. No task is Suspended or Deleted during normal operation; those states mean respectively excluded from scheduling until resumed, and removed with resources eligible for reclamation.")
p("vTaskDelayUntil() computes each wake time from the previous scheduled wake, keeping nominal sensor samples 2 seconds apart. vTaskDelay(2000) would add the execution time of each DHT/ADC cycle to every period, creating cumulative drift.")

section("4. Implementation")
p("board.cpp initializes HSI/PLL at 64 MHz, GPIO, ADC1, I2C1, USART1 and two timers. TIM2 supplies microsecond pulse timing for the DHT22. SensorTask pulls its open-drain data pin low for 2 ms, then captures 40 pulse widths in a FreeRTOS critical section, validates the checksum and converts tenths to Celsius and relative humidity. Invalid DHT data is shown as SENSOR ERROR and suppresses a false temperature alarm.")
p("The LDR module's AO voltage decreases as simulated illumination increases. The firmware maps the 12-bit ADC code inversely into an approximate 0-100% relative brightness scale; it does not infer calibrated lux. The encoder's CLK falling edge is handled by EXTI1; its DT level determines direction. DisplayTask alone builds a 1 KB page buffer and writes it over I2C1 to the SSD1306. TIM4 channel 3 generates a 500 Hz, 50% duty cycle buzzer waveform.")

section("5. Verification and Testing")
p("PlatformIO Core 6.2.0 built the bluepill_f103c8 STM32Cube target successfully. The latest local build used 20,628 of 65,536 flash bytes and 12,056 of 20,480 static RAM bytes. Fifteen native Unity tests passed: five alarm threshold cases, four navigation cases, four state transition cases and two brightness endpoints. Those tests exercise pure decisions; they do not test HAL timing or Wokwi wiring.")
table(["Functional tests", "Evidence state"], [
    ("FT-01 to FT-03: DHT/ADC display", "Pending interactive Wokwi observation"),
    ("FT-04 to FT-05: encoder navigation", "Pending interactive Wokwi observation"),
    ("FT-06 to FT-07: buzzer thresholds", "Pending interactive Wokwi observation"),
    ("FT-08 to FT-10: motion/inactivity", "Pending interactive Wokwi observation"),
], [2.7*inch, 3.95*inch])
p("The reproducible stimulus, expected output and actual-observation fields are in docs/verification.md. No Wokwi case has been marked PASS without observing it. The three deliberate experiments - removing blocking, raising a frequent task's priority, and bypassing the UART mutex - also remain pending and must be performed on a temporary branch, observed, and reverted.")

section("6. Static Code Analysis")
p("The first <b>pio check -e bluepill_f103c8</b> run passed with zero high, zero medium and seven low style messages. One local name shadowed a function and was renamed. A redundant display state assignment was removed. The latest rerun passed with zero high, zero medium and eight low messages after the Wokwi startup diagnostics were added. These C-style-cast reports point to expansion of STM32 HAL register or FreeRTOS macros at board.cpp, input.cpp and main.cpp call sites. They are in upstream macro definitions, not explicit casts in project source.")
table(["Finding", "Location / cause", "Resolution"], [
    ("Shadowed name", "display.cpp:55; local index", "Renamed to offset"),
    ("Redundant assignment", "display.cpp:134; screen flag", "Removed flag and assignment"),
    ("C-style casts (8)", "HAL / FreeRTOS macro expansion", "Retained; third-party macro style"),
], [1.4*inch, 2.65*inch, 2.6*inch])

section("7. Engineering Discussion")
p("The STM32F103C8 has only 20 KB RAM. A one-kilobyte static OLED framebuffer, one-slot mailboxes and small task stacks keep static RAM below 60% in the measured build. A queue-per-consumer costs some RAM but avoids lost sensor updates. DHT pulse capture disables interrupts for roughly 4 ms, which can disturb precise task latency; timer input capture would be preferable for a production design. The alarm is muted in INACTIVE according to the stated ACTIVE behavior, but a safety-critical monitor could choose to keep it active instead.")
p("The current evidence proves compilation, testable decision logic and static analysis only. Wokwi simulation timing, actual pin interactions and audible output require an interactive run. Physical deployment additionally needs voltage, pull-up, EMI, timing and thermal validation. Sensor calibration and fault-tolerant alarming are outside this laboratory prototype.")

section("8. Requirements Traceability")
table(["Req.", "Implementation", "Verification"], [
    ("FR-01", "SensorTask DHT temperature", "Unit alarm boundaries; FT-01 pending"),
    ("FR-02", "SensorTask DHT humidity", "FT-02 pending"),
    ("FR-03", "SensorTask ADC1 relative light", "Unit endpoints; FT-03 pending"),
    ("FR-04", "MotionTask PIR", "FT-08 and FT-10 pending"),
    ("FR-05", "DisplayTask OLED", "FT-01 to FT-03 pending"),
    ("FR-06", "InputTask encoder", "4 unit navigation cases; FT-04/05 pending"),
    ("FR-07", "AlarmTask PWM buzzer", "5 unit alarm cases; FT-06/07 pending"),
    ("FR-08", "StateTask ACTIVE/INACTIVE", "4 unit state cases; FT-08/09 pending"),
    ("FR-09", "StateTask 15 s timeout", "Unit timeout boundary; FT-09 pending"),
    ("FR-10", "StateTask PIR reactivation", "Unit reactivation; FT-10 pending"),
], [.55*inch, 2.45*inch, 3.65*inch])

section("9. Conclusion")
p("The project demonstrates a modular STM32Cube/FreeRTOS design with separate task responsibilities, intentional IPC, time-based state management and deterministic host tests. The remaining work is to run the Wokwi functional matrix and fault experiments, attach actual observations and circuit screenshots, and publish the verified results under the student's own accounts. The project should not be presented as fully simulator-verified until those steps are complete.")

section("References")
p("BCA182 Laboratory Activity 1 (MSU-IIT, 2026); PlatformIO STM32Cube documentation; Wokwi STM32 Blue Pill documentation; FreeRTOS kernel source distributed in STM32CubeF1.")


def footer(canvas, doc):
    canvas.saveState()
    canvas.setFont("Helvetica", 8)
    canvas.setFillColor(colors.HexColor("#536773"))
    canvas.drawString(0.75*inch, 0.45*inch, "BCA182 - Laboratory Activity 1")
    canvas.drawRightString(7.75*inch, 0.45*inch, f"Page {doc.page}")
    canvas.restoreState()


document = SimpleDocTemplate(str(OUT), pagesize=(8.5*inch, 11*inch),
                             rightMargin=.75*inch, leftMargin=.75*inch,
                             topMargin=.7*inch, bottomMargin=.7*inch,
                             title="BCA182 Laboratory Activity 1 Report",
                             author="Kent Alexis Alia")
document.build(story, onFirstPage=footer, onLaterPages=footer)
print(OUT)
