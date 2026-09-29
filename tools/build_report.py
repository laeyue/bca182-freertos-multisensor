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
styles.add(ParagraphStyle(name="SectionCustom", parent=styles["Heading1"], fontSize=11.5,
                          leading=14, spaceBefore=10, spaceAfter=5,
                          keepWithNext=True, textColor=colors.HexColor("#15344b")))
styles.add(ParagraphStyle(name="BodyCustom", parent=styles["BodyText"], fontSize=8.6,
                          leading=11.5, spaceAfter=6))
styles.add(ParagraphStyle(name="SmallCustom", parent=styles["BodyText"], fontSize=7.2,
                          leading=9))
styles.add(ParagraphStyle(name="SubCustom", parent=styles["Heading2"], fontSize=9.5,
                          leading=11, spaceBefore=6, spaceAfter=3))
styles.add(ParagraphStyle(name="CenteredCustom", parent=styles["BodyText"],
                          alignment=TA_CENTER, fontSize=9, leading=13, spaceAfter=6))

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
p("Name: Kent Alexis Alia&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Section: B182&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Date: 9/30/26")
p("Status: firmware build, 15 native tests, and static analysis passed. Wokwi verified DHT22 and LDR readings, all four OLED pages, PIR activity/inactivity, clockwise and counterclockwise page wrap, and the high-temperature alarm display/buzzer response. The retained PB8 trace shows PWM start and recovery. Physical sound level and physical-board behavior were not measured. Two reversible scheduling faults starved sensor output; the UART mutex-bypass trial did not reproduce interleaving.")

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
p("board.cpp initializes HSI/PLL at 64 MHz, GPIO, ADC1, I2C1, USART1 and two timers. TIM2/DWT timing supports the DHT22 transaction. SensorTask pulls its open-drain data pin low for 2 ms, then samples each data bit 40 us after its rising edge inside a FreeRTOS critical section. This midpoint separates the AM2302 zero pulse (26-28 us high) from its approximately 70 us one pulse. The task validates the checksum and converts tenths to Celsius and relative humidity. Invalid DHT data is shown as SENSOR ERROR and suppresses a false temperature alarm.")
p("The LDR module's AO voltage decreases as simulated illumination increases. The firmware maps the 12-bit ADC code inversely into an approximate 0-100% relative brightness scale; it does not infer calibrated lux. The encoder's CLK falling edge is handled by EXTI1; its DT level determines direction. DisplayTask alone builds a 1 KB page buffer and writes it over I2C1 to the SSD1306. TIM4 channel 3 generates a 500 Hz, 50% duty cycle buzzer waveform.")

section("5. Verification and Testing")
p("The bluepill_f103c8 STM32Cube firmware build used 21,620 of 65,536 flash bytes and 12,060 of 20,480 static RAM bytes. All 15 native Unity tests passed: five alarm thresholds, four navigation transitions, four state transitions and two brightness endpoints. Static analysis reported zero high, zero medium and 16 low C-style cast findings at STM32 HAL or FreeRTOS macro call sites. These checks do not test HAL timing or wiring.")
table(["Functional tests", "Evidence state"], [
    ("DHT22 acquisition", "Checksum-valid Wokwi serial reads at 24 C / 40%, 27.8 C / 65%, 32 C / 65%, and 45.8 C / 65%; revised 40 us midpoint decoder"),
    ("FT-01 to FT-03: OLED sensor pages", "PASS: Temperature 27.8 C, Humidity 65.0%, and Light page rendered; corresponding screenshots are in docs/evidence/"),
    ("FT-04 to FT-05: encoder navigation", "PASS: while ACTIVE, Wokwi showed Temperature -> Humidity -> Light -> Motion -> Temperature clockwise and the reverse sequence counterclockwise, with page-change logs"),
    ("FT-06 to FT-07: buzzer thresholds", "PASS in Wokwi: at 33.1 C / 66%, OLED showed ALARM and the simulator buzzer activity icon appeared; retained PB8 trace is about 500 Hz and recovery trace returns low. Physical sound level was not measured"),
    ("FT-08 to FT-10: motion/inactivity", "PASS: PIR high produced Motion: detected and State: ACTIVE; the earlier timeout run showed State: INACTIVE and a blank OLED"),
    ("Fault experiments", "No delay and high-priority MotionTask variants suppressed sensor output; bypassing the UART mutex did not produce visible interleaving in the short run"),
], [2.7*inch, 3.95*inch])
p("Separate OLED screenshots show all four pages; LDR output changed from 76% at 501 lux to 97% at 13,183 lux. Checksum-valid DHT samples ranged from 24 C to 45.8 C. At 32 C, the PB8 VCD shows 4,096 transitions at about 500 Hz; after a valid 23.5 C sample it stayed low. PIR activation logged ACTIVE, and a separate timeout test logged INACTIVE with the OLED blank. On 2026-09-30, a live VS Code Wokwi run showed the clockwise page sequence Temperature -> Humidity -> Light -> Motion -> Temperature and the reverse sequence, with InputTask page-change logs. At 33.1 C / 66.0%, serial showed a valid sensor sample, the OLED displayed ALARM, and the Wokwi buzzer activity icon appeared. The temporary test DHT and extended PIR hold settings were restored to defaults after the run. No new screenshot was retained; the text run record and retained traces are in docs/evidence/. Physical sound level and physical-board behavior were not measured. No-delay and high-priority faults suppressed sensor output; the UART mutex-bypass test showed no visible interleaving.")

section("6. Static Code Analysis")
p("The latest <b>pio check</b> run passed with zero high, zero medium and 16 low style messages. These C-style-cast reports point to STM32 HAL register or FreeRTOS macro expansions at board.cpp, input.cpp and main.cpp call sites; no high or medium finding was reported.")
table(["Finding", "Location / cause", "Resolution"], [
    ("C-style casts (16)", "HAL / FreeRTOS macro expansion", "Retained; low-severity macro style"),
], [1.4*inch, 2.65*inch, 2.6*inch])

section("7. Engineering Discussion")
p("The STM32F103C8 has only 20 KB RAM. A one-kilobyte static OLED framebuffer, one-slot mailboxes and small task stacks keep static RAM below 60% in the measured build. A queue-per-consumer costs some RAM but avoids lost sensor updates. DHT pulse capture disables interrupts for roughly 4 ms, which can disturb precise task latency; timer input capture would be preferable for a production design. The alarm is muted in INACTIVE according to the stated ACTIVE behavior, but a safety-critical monitor could choose to keep it active instead.")
p("The evidence includes Wokwi checks for DHT22 values through 45.8 C, LDR response, four OLED pages, both encoder page sequences with wraparound, PIR active/inactive transitions, and the 500 Hz buzzer PWM start and recovery, in addition to build, unit tests and static analysis. The 2026-09-30 live run also showed the high-temperature ALARM page and the Wokwi buzzer activity icon. The no-delay and high-priority MotionTask faults suppressed sensor output. The UART mutex-bypass trial showed no visible interleaving, so that race was not reproduced. Physical deployment still needs voltage, pull-up, EMI, timing, acoustic and thermal validation. Sensor calibration and fault-tolerant alarming are outside this laboratory prototype.")

section("8. Requirements Traceability")
table(["Req.", "Implementation", "Verification"], [
    ("FR-01", "SensorTask DHT temperature", "Valid Wokwi readings observed through 45.8 C; OLED FT-01 pass at 27.8 C"),
    ("FR-02", "SensorTask DHT humidity", "Wokwi serial value and OLED FT-02 pass at 65.0%"),
    ("FR-03", "SensorTask ADC1 relative light", "Changed Wokwi ADC readings and OLED FT-03 pass"),
    ("FR-04", "MotionTask PIR", "PASS: motion detected and ACTIVE serial logs observed; high/low trace captured"),
    ("FR-05", "DisplayTask OLED", "Temperature, Humidity, Light and Motion pages rendered in Wokwi"),
    ("FR-06", "InputTask encoder", "PASS: Wokwi showed ordered page navigation and wrap in both directions while ACTIVE"),
    ("FR-07", "AlarmTask PWM buzzer", "PASS for PB8 PWM start and stop: about 500 Hz at 32 C, low after 23.5 C; no acoustic measurement"),
    ("FR-08", "StateTask ACTIVE/INACTIVE", "PASS: ACTIVE and INACTIVE state logs observed with OLED off/on behavior"),
    ("FR-09", "StateTask 15 s timeout", "Unit timeout and Wokwi INACTIVE observation; FT-09 pass"),
    ("FR-10", "StateTask PIR reactivation", "PASS: PIR trigger after inactivity produced Motion: detected and State: ACTIVE"),
], [.55*inch, 2.45*inch, 3.65*inch])

section("9. Conclusion")
p("The project demonstrates a modular STM32Cube/FreeRTOS design with separate task responsibilities, intentional IPC, time-based state management and deterministic host tests. Wokwi verified DHT22 acquisition up to 45.8 C, LDR response, all four OLED pages, PIR activity/inactivity, both ordered encoder page sequences with wraparound, and the high-temperature OLED ALARM/buzzer response. The fault experiments reproduced starvation when a frequent MotionTask did not block or ran at excessive priority; the short UART mutex-bypass trial did not reproduce a corrupted line. Physical sound level and physical-board behavior remain unmeasured.")

section("References")
p("BCA182 Laboratory Activity 1 (MSU-IIT, 2026); Aosong AM2302 Technical Manual; PlatformIO STM32Cube documentation; Wokwi STM32 Blue Pill documentation; FreeRTOS kernel source distributed in STM32CubeF1.")


def footer(canvas, doc):
    canvas.saveState()
    canvas.setFont("Helvetica", 8)
    canvas.setFillColor(colors.HexColor("#536773"))
    canvas.drawString(0.75*inch, 0.45*inch, "BCA182 - Laboratory Activity 1")
    canvas.drawRightString(7.75*inch, 0.45*inch, f"Page {doc.page}")
    canvas.restoreState()


document = SimpleDocTemplate(str(OUT), pagesize=(8.5*inch, 11*inch),
                             rightMargin=.72*inch, leftMargin=.72*inch,
                             topMargin=.65*inch, bottomMargin=.65*inch,
                             title="BCA182 Laboratory Activity 1 Report",
                             author="Kent Alexis Alia")
document.build(story, onFirstPage=footer, onLaterPages=footer)
print(OUT)
