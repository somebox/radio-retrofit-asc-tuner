# Radio Retrofit Refactoring Plan

## Current State Analysis

### Existing Codebase Structure
- **RetroText-based**: Full-featured display system with SignTextController, DisplayManager, fonts, WiFi, clock, animations
- **Hardware**: 3x RetroText modules (18 characters total), IS31FL3737 drivers at addresses 0x50, 0x51, 0x52
- **Features**: Demo modes, smooth/character scrolling, clock display, meteor animations, WiFi time sync

### Target Hardware Configuration
- **Display**: 3x RetroText modules, 18 4x6 pixel characters, I2C addresses 0x50, 0x51, 0x52 (GND, VCC, SDA)
- **Keypad**: 4x6 button matrix via TCA8418 controller, I2C address 0x53 (SCL)
- **Rotary Encoder**: Push-button encoder via TCA8418
- **Power**: 5V external supply

### Gap Analysis
- Missing TCA8418 keypad integration
- Demo modes don't match radio use cases
- No rotary encoder support
- Mode system doesn't align with radio functionality

## Refactoring Strategy

### Phase 1: Hardware Integration
#### 1.1 Keypad Controller Integration
- Add TCA8418 initialization and configuration
- Implement 4x6 button matrix scanning
- Add rotary encoder support through TCA8418
- Create button event handling system

#### 1.2 Hardware Abstraction Layer
- Create `RadioHardware` class to manage all I2C devices
- Consolidate I2C device initialization and error handling
- Implement device health monitoring and recovery

### Phase 2: Mode System Redesign
#### 2.1 Radio-Specific Modes
- Replace demo modes with radio operation modes:
  - `STARTUP`: Component testing and initialization
  - `PRESET`: Preset selection (default mode)
  - `PLAYLIST`: Detailed playlist browsing
  - `SOURCE`: Source selection (internet radio, local, spotify, bluetooth)
  - `SETTINGS`: Configuration and settings

#### 2.2 State Management
- Implement proper state machine with transitions
- Add timeout handling for mode switches
- Create visual feedback system for mode changes
- Implement button behavior context switching

### Phase 3: User Interface Framework
#### 3.1 Input Handling System
- Create `InputManager` class for unified input handling
- Implement button mapping system (context-dependent)
- Add rotary encoder navigation support
- Create debouncing and multi-press detection

#### 3.2 Display Management Refactor
- Refactor DisplayManager for radio-specific text layouts
- Add preset display formatting
- Implement scrolling playlist views
- Create status indicators and progress bars

### Phase 4: Library Structure
#### 4.1 Component Separation
- Extract core radio functionality into separate classes:
  - `RadioController`: Main state machine and coordination
  - `PresetManager`: Preset storage and selection
  - `PlaylistManager`: Playlist navigation and display
  - `SourceManager`: Source switching and configuration

#### 4.2 ESPHome Integration Preparation
- Create clean API interfaces for external control
- Implement event callback system
- Add configuration parameter system
- Design for library packaging

### Phase 5: Feature Implementation
#### 5.1 Preset System
- Implement 6-preset button handling
- Add preset storage and recall
- Create preset display formatting
- Add preset programming functionality

#### 5.2 Playlist Navigation
- Implement rotary encoder scrolling
- Add playlist item display formatting
- Create selection and playback controls
- Add search and filtering capabilities

#### 5.3 Source Management
- Implement source switching logic
- Add source-specific configuration
- Create source status display
- Add connection management

## Implementation Priority

### High Priority (Core Functionality)
1. TCA8418 keypad integration
2. Radio mode system implementation
3. Basic preset button handling
4. Hardware abstraction layer

### Medium Priority (User Experience)
1. Rotary encoder navigation
2. Display formatting improvements
3. State machine refinement
4. Error handling and recovery

### Low Priority (Advanced Features)
1. ESPHome library structure
2. Advanced playlist features
3. Configuration persistence
4. Performance optimizations

## File Structure Changes

### New Files
```
include/
  RadioController.h
  InputManager.h
  PresetManager.h
  PlaylistManager.h
  SourceManager.h
  RadioHardware.h

src/
  RadioController.cpp
  InputManager.cpp
  PresetManager.cpp
  PlaylistManager.cpp
  SourceManager.cpp
  RadioHardware.cpp
```

### Modified Files
```
src/main.cpp           # Simplified to radio-specific functionality
include/DisplayManager.h  # Radio-specific display methods
platformio.ini         # Updated dependencies
```

### Deprecated Files
```
src/MeteorAnimation.cpp    # Demo-specific, remove
include/MeteorAnimation.h  # Demo-specific, remove
```

## Testing Strategy

### Hardware Testing
- Component initialization verification
- I2C communication validation
- Button matrix functionality
- Display driver operation

### Integration Testing
- Mode switching validation
- Input-output coordination
- State persistence testing
- Error condition handling

### User Experience Testing
- Button responsiveness
- Display readability
- Navigation flow
- Timeout behavior
