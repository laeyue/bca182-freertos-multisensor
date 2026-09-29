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
p("Name: Kent Alexis Alia&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Section: B182&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Date: 9/29/26")
p("Status: firmware build, host tests, and selected Wokwi sensor checks verified; full OLED, encoder, alarm, PIR, and fault-experiment evidence remains pending.")

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
p("The bluepill_f103c8 STM32Cube build succeeded using 21,616 of 65,536 flash bytes and 12,060 of 20,480 static RAM bytes. All 15 native Unity tests passed: five alarm threshold cases, four navigation cases, four state transition cases and two brightness endpoints. The configured cppcheck run passed with zero high and medium findings and 16 low style findings. These checks do not test HAL timing or wiring.")
table(["Functional tests", "Evidence state"], [
    ("DHT22 acquisition", "Wokwi serial reads verified at 24 C / 40% and 32 C / 65%; revised 40 us midpoint decoder"),
    ("FT-01 to FT-03: OLED sensor pages", "Temperature page showed 32.0 C and ACTIVE; requested 28 C, Humidity and Light pages pending"),
    ("FT-04 to FT-05: encoder navigation", "Pending interactive Wokwi observation"),
    ("FT-06 to FT-07: buzzer thresholds", "DHT 32 C reading observed; buzzer boundary behavior pending"),
    ("FT-08 to FT-10: motion/inactivity", "INACTIVE observed; PIR trigger restored OLED; ACTIVE log and reactivation details pending"),
], [2.7*inch, 3.95*inch])
p("LDR serial output changed from 76% (ADC 980 at 501 lux) to 97% (ADC 130 at 13,183 lux). The Temperature page showed 32.0 C and ACTIVE after a PIR trigger; other page values remain untested. The inactivity test logged State: INACTIVE and the OLED blanked. The original startup delay symptom was not repeated in the final reload run: sensor samples and MotionTask heartbeats continued. The full-circuit and OLED screenshots are in docs/evidence/. The reproducible stimulus and actual-observation fields are in docs/verification.md. FT-09 is the only end-to-end test currently marked PASS there; other cases remain pending where the expected display or buzzer behavior was not captured. The three deliberate scheduling/UART fault experiments remain pending.")

section("6. Static Code Analysis")
p("The latest <b>pio check</b> run passed with zero high, zero medium and 16 low style messages. These C-style-cast reports point to STM32 HAL register or FreeRTOS macro expansions at board.cpp, input.cpp and main.cpp call sites; no high or medium finding was reported.")
table(["Finding", "Location / cause", "Resolution"], [
    ("C-style casts (16)", "HAL / FreeRTOS macro expansion", "Retained; low-severity macro style"),
], [1.4*inch, 2.65*inch, 2.6*inch])

section("7. Engineering Discussion")
p("The STM32F103C8 has only 20 KB RAM. A one-kilobyte static OLED framebuffer, one-slot mailboxes and small task stacks keep static RAM below 60% in the measured build. A queue-per-consumer costs some RAM but avoids lost sensor updates. DHT pulse capture disables interrupts for roughly 4 ms, which can disturb precise task latency; timer input capture would be preferable for a production design. The alarm is muted in INACTIVE according to the stated ACTIVE behavior, but a safety-critical monitor could choose to keep it active instead.")
p("The evidence includes selected interactive Wokwi checks for DHT22 values, LDR response, the active Temperature display and inactivity in addition to build, unit tests and static analysis. Other OLED pages, full PIR reactivation logs and audible alarm behavior remain to be confirmed. Physical deployment additionally needs voltage, pull-up, EMI, timing and thermal validation. Sensor calibration and fault-tolerant alarming are outside this laboratory prototype.")

section("8. Requirements Traceability")
table(["Req.", "Implementation", "Verification"], [
    ("FR-01", "SensorTask DHT temperature", "Wokwi serial values observed; OLED FT-01 pending"),
    ("FR-02", "SensorTask DHT humidity", "Wokwi serial values observed; OLED FT-02 pending"),
    ("FR-03", "SensorTask ADC1 relative light", "Wokwi ADC response observed; OLED FT-03 pending"),
    ("FR-04", "MotionTask PIR", "FT-08 and FT-10 pending"),
    ("FR-05", "DisplayTask OLED", "Temperature page observed at 32.0 C; requested 28 C/Humidity/Light pages pending"),
    ("FR-06", "InputTask encoder", "4 unit navigation cases; FT-04/05 pending"),
    ("FR-07", "AlarmTask PWM buzzer", "5 unit alarm cases; FT-06/07 pending"),
    ("FR-08", "StateTask ACTIVE/INACTIVE", "Unit state cases; FT-09 observed, FT-08 pending"),
    ("FR-09", "StateTask 15 s timeout", "Unit timeout and Wokwi INACTIVE observation; FT-09 pass"),
    ("FR-10", "StateTask PIR reactivation", "Unit reactivation; FT-10 pending"),
], [.55*inch, 2.45*inch, 3.65*inch])

section("9. Conclusion")
p("The project demonstrates a modular STM32Cube/FreeRTOS design with separate task responsibilities, intentional IPC, time-based state management and deterministic host tests. DHT22 acquisition, LDR response, the active Temperature display and the inactivity timeout were observed in Wokwi. Remaining work includes the other OLED pages, encoder directions, audible alarm response, complete PIR reactivation logs and the three fault experiments. This local report is not a claim that the entire circuit is simulator-verified.")

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
