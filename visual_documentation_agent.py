#!/usr/bin/env python3
"""
Visual Documentation Agent
Transforms technical guides into visually appealing, easy-to-read formats.

Usage:
    python visual_documentation_agent.py input.md output.md --format iot
"""

import re
import json
import argparse
from typing import Dict, List, Optional
from dataclasses import dataclass, field
from enum import Enum


class DiagramType(Enum):
    """Supported diagram types"""
    FLOWCHART = "flowchart"
    SEQUENCE = "sequence"
    CLASS = "class"
    STATE = "state"
    ER = "er"
    NETWORK = "network"
    TIMELINE = "timeline"
    JOURNEY = "journey"


class OutputFormat(Enum):
    """Supported output formats"""
    MARKDOWN = "markdown"
    HTML = "html"
    PDF = "pdf"


@dataclass
class Component:
    """Hardware or software component"""
    name: str
    model: str
    quantity: int
    purpose: str
    specs: str = ""
    cost: str = ""
    alternatives: List[str] = field(default_factory=list)


@dataclass
class Connection:
    """Hardware connection details"""
    from_pin: str
    to_pin: str
    component: str
    signal: str
    wire_color: str = ""


@dataclass
class CodeSnippet:
    """Code example with annotations"""
    language: str
    code: str
    annotations: Dict[str, str] = field(default_factory=dict)
    line_numbers: bool = True


class VisualDocumentationAgent:
    """Main agent class for transforming technical documentation"""

    def __init__(self, config_path: Optional[str] = None):
        """Initialize the agent with optional configuration"""
        self.config = self._load_config(config_path)
        self.colors = self.config.get("colors", {})
        self.typography = self.config.get("typography", {})

    def _load_config(self, config_path: Optional[str]) -> Dict:
        """Load configuration from JSON file"""
        if config_path:
            with open(config_path, 'r') as f:
                return json.load(f)
        return self._default_config()

    def _default_config(self) -> Dict:
        """Return default configuration"""
        return {
            "colors": {
                "primary": "#2196F3",
                "secondary": "#4CAF50",
                "accent": "#FF9800",
                "error": "#f44336",
                "neutral": "#607D8B"
            },
            "typography": {
                "heading_font": "Inter, Roboto, system-ui",
                "code_font": "Fira Code, JetBrains Mono",
                "line_height": 1.6
            }
        }

    def transform(self, input_text: str, format_type: str = "iot") -> str:
        """
        Transform input technical text into visual documentation

        Args:
            input_text: Raw technical guide text
            format_type: Type of documentation (iot, api, tutorial, etc.)

        Returns:
            Formatted markdown with visual elements
        """
        # Parse input
        parsed_content = self._parse_input(input_text)

        # Generate visual structure
        visual_doc = self._generate_visual_structure(parsed_content, format_type)

        return visual_doc

    def _parse_input(self, input_text: str) -> Dict:
        """Parse input text into structured data"""
        lines = input_text.split('\n')
        parsed = {
            "title": "",
            "overview": "",
            "components": [],
            "connections": [],
            "code_blocks": [],
            "sections": []
        }

        current_section = None
        code_buffer = []

        for line in lines:
            # Title
            if line.startswith('# '):
                parsed["title"] = line[2:].strip()

            # Components section
            elif "Parts needed" in line or "Components" in line:
                current_section = "components"

            # Wiring section
            elif "Wiring" in line or "Pinout" in line:
                current_section = "connections"

            # Code section
            elif line.strip().startswith("```") and current_section != "code":
                current_section = "code"
                language = line.strip().replace("```", "").strip()
                code_buffer = []
                parsed["code_blocks"].append({
                    "language": language if language else "cpp",
                    "code": ""
                })

            elif current_section == "code" and line.strip().startswith("```"):
                current_section = None

            elif current_section == "code":
                code_buffer.append(line)
                if parsed["code_blocks"]:
                    parsed["code_blocks"][-1]["code"] = '\n'.join(code_buffer)

        return parsed

    def _generate_visual_structure(self, parsed: Dict, format_type: str) -> str:
        """Generate the complete visual documentation structure"""
        output = []

        # Title and hero
        output.append(f"# {parsed.get('title', 'Project Documentation')}\n")
        output.append("*Transform your technical content into beautiful documentation*\n")
        output.append("---\n")

        # Quick overview
        output.append(self._generate_quick_overview(parsed))

        # Table of contents
        output.append(self._generate_toc())

        # System architecture (if IoT)
        if format_type == "iot":
            output.append(self._generate_architecture_section())

        # Components gallery
        if parsed.get("components"):
            output.append(self._generate_components_section())

        # Circuit diagrams
        if format_type == "iot":
            output.append(self._generate_circuit_section())

        # Code section
        if parsed.get("code_blocks"):
            output.append(self._generate_code_section(parsed["code_blocks"]))

        # Troubleshooting
        output.append(self._generate_troubleshooting_section())

        # Next steps
        output.append(self._generate_next_steps_section())

        return '\n'.join(output)

    def _generate_quick_overview(self, parsed: Dict) -> str:
        """Generate quick overview section"""
        return """
## Quick Overview

> A brief summary of the project and its objectives.

**Difficulty Level**: Beginner | **Time Required**: 2-3 hours | **Cost**: ~$25

---

"""

    def _generate_toc(self) -> str:
        """Generate table of contents"""
        return """
## Table of Contents

1. [Introduction](#introduction)
2. [System Architecture](#system-architecture)
3. [Hardware Components](#hardware-components)
4. [Circuit Diagram](#circuit-diagram)
5. [Software Setup](#software-setup)
6. [Step-by-Step Guide](#step-by-step-guide)
7. [Testing & Troubleshooting](#testing--troubleshooting)
8. [Next Steps](#next-steps)

---

"""

    def _generate_architecture_section(self) -> str:
        """Generate system architecture section with Mermaid diagram"""
        return """
## 1. System Architecture

### High-Level Architecture

```mermaid
graph TB
    subgraph "Physical Layer"
        A[Sensors] --> B[Microcontroller]
        B --> C[Actuators]
    end

    subgraph "Communication Layer"
        B --> D[Communication Module]
        D --> E[Cloud/Gateway]
    end

    subgraph "Application Layer"
        E --> F[Database]
        E --> G[API]
        G --> H[Web Dashboard]
    end

    style A fill:#e1f5ff
    style B fill:#fff4e1
    style C fill:#e1f5ff
    style E fill:#ffe1f5
```

### Data Flow

```mermaid
sequenceDiagram
    participant Sensor
    participant MCU
    participant Cloud
    participant User

    Sensor->>MCU: Read Data
    MCU->>MCU: Process Data
    MCU->>Cloud: Transmit
    Cloud->>User: Display
```

---

"""

    def _generate_components_section(self) -> str:
        """Generate components section with comparison table"""
        return """
## 2. Hardware Components

### Component Comparison

| Component | Model | Quantity | Purpose | Specs | Cost |
|-----------|-------|----------|---------|-------|------|
| Microcontroller | ESP32 | 1 | Central processing | 240MHz dual-core | $5 |
| Sensor | DHT22 | 1 | Temperature/Humidity | 0-100% RH | $5 |
| Display | OLED 128x64 | 1 | Show readings | I2C interface | $8 |
| Power | USB/Battery | 1 | Power system | 5V input | $2 |

---

"""

    def _generate_circuit_section(self) -> str:
        """Generate circuit diagram section"""
        return """
## 3. Circuit Diagram

### Wiring Overview

```mermaid
graph LR
    ESP32[ESP32<br/>Microcontroller] -->|GPIO 4| DHT[DHT22<br/>Sensor]
    ESP32 -->|I2C SDA| OLED[OLED<br/>Display]
    ESP32 -->|I2C SCL| OLED
    ESP32 -->|5V| VCC[Power<br/>Supply]

    style ESP32 fill:#4CAF50,color:#fff
    style DHT fill:#2196F3,color:#fff
    style OLED fill:#FF9800,color:#fff
```

### Connection Table

| From (ESP32) | To (Component) | Pin | Wire Color |
|--------------|---------------|-----|------------|
| GPIO 4 | DHT22 | Data | Yellow |
| 3.3V | DHT22 | VCC | Red |
| GND | DHT22 | GND | Black |
| GPIO 21 | OLED | SDA | Green |
| GPIO 22 | OLED | SCL | Blue |

---

"""

    def _generate_code_section(self, code_blocks: List[Dict]) -> str:
        """Generate code section with syntax highlighting and annotations"""
        output = ["## 4. Software Setup\n"]
        output.append("### Complete Firmware Code\n")

        for block in code_blocks:
            language = block.get("language", "cpp")
            code = block.get("code", "")

            output.append(f"```{language}\n{code}\n```\n")

            output.append("**Code Annotations:**\n\n")
            output.append("<details>\n")
            output.append("<summary><strong>Line-by-Line Explanation</strong></summary>\n\n")
            output.append("*Detailed explanations will be added here*\n\n")
            output.append("</details>\n")

        output.append("---\n")
        return '\n'.join(output)

    def _generate_troubleshooting_section(self) -> str:
        """Generate troubleshooting section with flowchart"""
        return """
## 5. Testing & Troubleshooting

### Common Issues

| Problem | Possible Cause | Solution |
|---------|---------------|----------|
| No sensor readings | Loose connection | Check wiring |
| Display blank | Power issue | Verify 3.3V supply |
| Wrong values | Wrong config | Update code |

### Troubleshooting Flowchart

```mermaid
flowchart TD
    A[Issue Detected] --> B{System Powers On?}
    B -->|No| C[Check Power Supply]
    B -->|Yes| D{Sensor Reading?}

    C --> E[Verify Connections]
    D -->|No| F[Check Wiring]
    D -->|Yes| G{Display Working?}

    F --> H[Verify Pin Configuration]
    G -->|No| I[Check I2C]

    style A fill:#ffcccc
    style H fill:#e1ffe1
    style I fill:#e1ffe1
```

---

"""

    def _generate_next_steps_section(self) -> str:
        """Generate next steps and learning roadmap"""
        return """
## 6. Next Steps

### Project Enhancements

```mermaid
timeline
    title Learning Path
    section Current
        Basic Implementation : Core functionality
    section Next
        Add WiFi : Cloud connectivity
    section Future
        Machine Learning : Advanced features
```

### Learning Resources

- [Official Documentation](https://example.com)
- [Community Forum](https://forum.example.com)
- [Video Tutorials](https://youtube.com/example)

---

## Support

Need help? Join our community:
- Discord: [Link]
- GitHub: [Link]
- Email: support@example.com

---

**Did you find this helpful?** [Rate Guide] | [Report Issue]
"""

    def generate_diagram(self, diagram_type: DiagramType, elements: List[Dict]) -> str:
        """
        Generate a Mermaid diagram

        Args:
            diagram_type: Type of diagram to generate
            elements: List of diagram elements (nodes, edges, etc.)

        Returns:
            Mermaid diagram code block
        """
        if diagram_type == DiagramType.FLOWCHART:
            return self._generate_flowchart(elements)
        elif diagram_type == DiagramType.SEQUENCE:
            return self._generate_sequence_diagram(elements)
        elif diagram_type == DiagramType.CLASS:
            return self._generate_class_diagram(elements)
        elif diagram_type == DiagramType.TIMELINE:
            return self._generate_timeline(elements)
        else:
            return self._generate_generic_diagram(diagram_type, elements)

    def _generate_flowchart(self, elements: List[Dict]) -> str:
        """Generate flowchart diagram"""
        lines = ["```mermaid", "flowchart TD"]

        for element in elements:
            if element.get("type") == "node":
                label = element.get("label", "")
                style = element.get("style", "")
                lines.append(f'    {element["id"]}[{label}]')
                if style:
                    lines.append(f'    style {element["id"]} {style}')
            elif element.get("type") == "edge":
                lines.append(f'    {element["from"]} --> {element["to"]}')

        lines.append("```")
        return '\n'.join(lines)

    def _generate_sequence_diagram(self, elements: List[Dict]) -> str:
        """Generate sequence diagram"""
        lines = ["```mermaid", "sequenceDiagram"]

        # Add participants
        for element in elements:
            if element.get("type") == "participant":
                lines.append(f'    participant {element["id"]} as {element.get("label", element["id"])}')

        # Add messages
        for element in elements:
            if element.get("type") == "message":
                lines.append(f'    {element["from"]} ->> {element["to"]}: {element.get("label", "")}')

        lines.append("```")
        return '\n'.join(lines)

    def _generate_class_diagram(self, elements: List[Dict]) -> str:
        """Generate class diagram"""
        lines = ["```mermaid", "classDiagram"]

        for element in elements:
            if element.get("type") == "class":
                lines.append(f'    class {element["name"]} {{')
                for attr in element.get("attributes", []):
                    lines.append(f'        {attr}')
                lines.append('    }')

        lines.append("```")
        return '\n'.join(lines)

    def _generate_timeline(self, elements: List[Dict]) -> str:
        """Generate timeline diagram"""
        lines = ["```mermaid", "timeline", "    title Project Roadmap"]

        current_section = None
        for element in elements:
            if element.get("type") == "section":
                current_section = element.get("name")
                lines.append(f'    section {current_section}')
            elif element.get("type") == "event":
                lines.append(f'        {element.get("name")} : {element.get("description", "")}')

        lines.append("```")
        return '\n'.join(lines)

    def _generate_generic_diagram(self, diagram_type: DiagramType, elements: List[Dict]) -> str:
        """Generate generic diagram for other types"""
        lines = ["```mermaid", f"{diagram_type.value} TD"]

        for element in elements:
            if element.get("type") == "node":
                lines.append(f'    {element["id"]}[{element.get("label", "")}]')
            elif element.get("type") == "edge":
                lines.append(f'    {element["from"]} --> {element["to"]}')

        lines.append("```")
        return '\n'.join(lines)


def main():
    """Main entry point for CLI usage"""
    parser = argparse.ArgumentParser(
        description="Transform technical guides into visual documentation"
    )
    parser.add_argument(
        "input_file",
        help="Input markdown file"
    )
    parser.add_argument(
        "output_file",
        help="Output markdown file"
    )
    parser.add_argument(
        "--format",
        choices=["iot", "api", "tutorial"],
        default="iot",
        help="Type of documentation to generate"
    )
    parser.add_argument(
        "--config",
        help="Path to configuration JSON file"
    )

    args = parser.parse_args()

    # Read input
    with open(args.input_file, 'r') as f:
        input_text = f.read()

    # Transform
    agent = VisualDocumentationAgent(args.config)
    output = agent.transform(input_text, args.format)

    # Write output
    with open(args.output_file, 'w') as f:
        f.write(output)

    print(f"✓ Visual documentation generated: {args.output_file}")


if __name__ == "__main__":
    main()
