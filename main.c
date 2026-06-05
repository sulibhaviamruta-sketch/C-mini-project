#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define MAX_OBJECTS 100
#define DEFAULT_WIDTH 80
#define DEFAULT_HEIGHT 25
// ANSI Color Escape Sequences for Rich Aesthetics
#define COLOR_RESET   "\033[0m"
#define COLOR_RULER   "\033[36m"   // Cyan
#define COLOR_BG      "\033[90m"   // Dark Gray (for grid background)
#define COLOR_SHAPE   "\033[92m"   // Bright Green (for drawn shapes)
#define COLOR_MENU    "\033[97m"   // Bright White
#define COLOR_ERROR   "\033[91m"   // Red
#define COLOR_INFO    "\033[93m"   // Yellow
typedef enum {
    SHAPE_LINE,
    SHAPE_RECTANGLE,
    SHAPE_CIRCLE,
    SHAPE_TRIANGLE
} ShapeType;
typedef struct {
    int x1, y1;
    int x2, y2;
} LineParams;
typedef struct {
    int x, y;
    int width, height;
} RectParams;
typedef struct {
    int cx, cy;
    int r;
} CircleParams;
typedef struct {
    int x1, y1;
    int x2, y2;
    int x3, y3;
} TriangleParams;
typedef struct {
    ShapeType type;
    union {
        LineParams line;
        RectParams rect;
        CircleParams circle;
        TriangleParams triangle;
    } params;
    char draw_char;
} Shape;
typedef struct {
    int width;
    int height;
    char bg_char;
    char canvas[DEFAULT_HEIGHT][DEFAULT_WIDTH];
    Shape objects[MAX_OBJECTS];
    int object_count;
} EditorState;
// Safe console inputs
int read_int(const char* prompt, int min_val, int max_val) {
    char buf[128];
    int val;
    while (1) {
        printf("%s", prompt);
        if (!fgets(buf, sizeof(buf), stdin)) {
            continue;
        }
        buf[strcspn(buf, "\n")] = 0; // Remove trailing newline
        
        char *endptr;
        long parsed = strtol(buf, &endptr, 10);
        if (endptr == buf || *endptr != '\0') {
            printf(COLOR_ERROR "Invalid input. Please enter a valid integer." COLOR_RESET "\n");
            continue;
        }
        if (parsed < min_val || parsed > max_val) {
            printf(COLOR_ERROR "Value out of range [%d, %d]. Try again." COLOR_RESET "\n", min_val, max_val);
            continue;
        }
        val = (int)parsed;
        break;
    }
    return val;
}
char read_char(const char* prompt, const char* allowed) {
    char buf[128];
    while (1) {
        printf("%s", prompt);
        if (!fgets(buf, sizeof(buf), stdin)) {
            continue;
        }
        buf[strcspn(buf, "\n")] = 0;
        if (strlen(buf) != 1) {
            printf(COLOR_ERROR "Please enter exactly one character." COLOR_RESET "\n");
            continue;
        }
        char c = buf[0];
        if (allowed && strchr(allowed, c) == NULL) {
            printf(COLOR_ERROR "Character not allowed. Choose from: %s" COLOR_RESET "\n", allowed);
            continue;
        }
        return c;
    }
}
// Drawing helper algorithms
void draw_line(char canvas[DEFAULT_HEIGHT][DEFAULT_WIDTH], int width, int height, int x1, int y1, int x2, int y2, char c) {
    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;
    while (1) {
        if (x1 >= 0 && x1 < width && y1 >= 0 && y1 < height) {
            canvas[y1][x1] = c;
        }
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}
void draw_circle(char canvas[DEFAULT_HEIGHT][DEFAULT_WIDTH], int width, int height, int xc, int yc, int r, char c) {
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;
    
    // We adjust drawing macro to check bounds dynamically
    #define PLOT(px, py) do { \
        if ((px) >= 0 && (px) < width && (py) >= 0 && (py) < height) \
            canvas[py][px] = c; \
    } while(0)
    while (y >= x) {
        PLOT(xc + x, yc + y);
        PLOT(xc - x, yc + y);
        PLOT(xc + x, yc - y);
        PLOT(xc - x, yc - y);
        PLOT(xc + y, yc + x);
        PLOT(xc - y, yc + x);
        PLOT(xc + y, yc - x);
        PLOT(xc - y, yc - x);
        
        if (d < 0) {
            d = d + 4 * x + 6;
        } else {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
    #undef PLOT
}
void clear_canvas(char canvas[DEFAULT_HEIGHT][DEFAULT_WIDTH], int width, int height, char bg_char) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            canvas[y][x] = bg_char;
        }
    }
}
void render_canvas(EditorState *state) {
    clear_canvas(state->canvas, state->width, state->height, state->bg_char);
    for (int i = 0; i < state->object_count; i++) {
        Shape *s = &state->objects[i];
        switch (s->type) {
            case SHAPE_LINE:
                draw_line(state->canvas, state->width, state->height,
                          s->params.line.x1, s->params.line.y1,
                          s->params.line.x2, s->params.line.y2, s->draw_char);
                break;
            case SHAPE_RECTANGLE: {
                int x = s->params.rect.x;
                int y = s->params.rect.y;
                int w = s->params.rect.width;
                int h = s->params.rect.height;
                // Draw four boundary segments
                draw_line(state->canvas, state->width, state->height, x, y, x + w - 1, y, s->draw_char);
                draw_line(state->canvas, state->width, state->height, x, y + h - 1, x + w - 1, y + h - 1, s->draw_char);
                draw_line(state->canvas, state->width, state->height, x, y, x, y + h - 1, s->draw_char);
                draw_line(state->canvas, state->width, state->height, x + w - 1, y, x + w - 1, y + h - 1, s->draw_char);
                break;
            }
            case SHAPE_CIRCLE:
                draw_circle(state->canvas, state->width, state->height,
                            s->params.circle.cx, s->params.circle.cy,
                            s->params.circle.r, s->draw_char);
                break;
            case SHAPE_TRIANGLE:
                draw_line(state->canvas, state->width, state->height,
                          s->params.triangle.x1, s->params.triangle.y1,
                          s->params.triangle.x2, s->params.triangle.y2, s->draw_char);
                draw_line(state->canvas, state->width, state->height,
                          s->params.triangle.x2, s->params.triangle.y2,
                          s->params.triangle.x3, s->params.triangle.y3, s->draw_char);
                draw_line(state->canvas, state->width, state->height,
                          s->params.triangle.x3, s->params.triangle.y3,
                          s->params.triangle.x1, s->params.triangle.y1, s->draw_char);
                break;
        }
    }
}
// Display Canvas with Coordinate Ruler Guides
void display_canvas(const EditorState *state) {
    // Print top tens ruler
    printf("   " COLOR_RULER);
    for (int x = 0; x < state->width; x++) {
        if (x % 10 == 0) {
            printf("%d", (x / 10) % 10);
        } else {
            printf(" ");
        }
    }
    printf(COLOR_RESET "\n");
    // Print top units ruler
    printf("   " COLOR_RULER);
    for (int x = 0; x < state->width; x++) {
        printf("%d", x % 10);
    }
    printf(COLOR_RESET "\n");
    // Top border
    printf("   +");
    for (int x = 0; x < state->width; x++) printf("-");
    printf("+\n");
    // Print rows with side ruler
    for (int y = 0; y < state->height; y++) {
        printf(COLOR_RULER "%02d" COLOR_RESET " |", y);
        
        char current_color = 0; // 0 = default, 1 = bg, 2 = shape
        for (int x = 0; x < state->width; x++) {
            char cell = state->canvas[y][x];
            if (cell == state->bg_char) {
                if (current_color != 1) {
                    printf(COLOR_BG);
                    current_color = 1;
                }
            } else {
                if (current_color != 2) {
                    printf(COLOR_SHAPE);
                    current_color = 2;
                }
            }
            putchar(cell);
        }
        printf(COLOR_RESET "|\n");
    }
    // Bottom border
    printf("   +");
    for (int x = 0; x < state->width; x++) printf("-");
    printf("+\n");
}
void print_shape_info(const Shape *s, int index) {
    switch (s->type) {
        case SHAPE_LINE:
            printf("[%d] Line: (%d, %d) to (%d, %d) [char: '%c']\n", 
                   index, s->params.line.x1, s->params.line.y1, 
                   s->params.line.x2, s->params.line.y2, s->draw_char);
            break;
        case SHAPE_RECTANGLE:
            printf("[%d] Rectangle: top-left (%d, %d), size %dx%d [char: '%c']\n", 
                   index, s->params.rect.x, s->params.rect.y, 
                   s->params.rect.width, s->params.rect.height, s->draw_char);
            break;
        case SHAPE_CIRCLE:
            printf("[%d] Circle: center (%d, %d), radius %d [char: '%c']\n", 
                   index, s->params.circle.cx, s->params.circle.cy, 
                   s->params.circle.r, s->draw_char);
            break;
        case SHAPE_TRIANGLE:
            printf("[%d] Triangle: Vertices (%d, %d), (%d, %d), (%d, %d) [char: '%c']\n", 
                   index, s->params.triangle.x1, s->params.triangle.y1, 
                   s->params.triangle.x2, s->params.triangle.y2, 
                   s->params.triangle.x3, s->params.triangle.y3, s->draw_char);
            break;
    }
}
int main() {
    // Clear screen initially using ANSI code
    printf("\033[H\033[J");
    EditorState state;
    state.width = DEFAULT_WIDTH;
    state.height = DEFAULT_HEIGHT;
    state.bg_char = '_';
    state.object_count = 0;
    char default_draw_char = '*';
    while (1) {
        render_canvas(&state);
        
        printf(COLOR_MENU "\n=== 2D Graphics Editor (C Canvas) ===\n" COLOR_RESET);
        display_canvas(&state);
        printf("\n" COLOR_MENU "Active Shapes (%d/%d):" COLOR_RESET "\n", state.object_count, MAX_OBJECTS);
        if (state.object_count == 0) {
            printf("  (None)\n");
        } else {
            for (int i = 0; i < state.object_count; i++) {
                printf("  ");
                print_shape_info(&state.objects[i], i);
            }
        }
        printf("\n" COLOR_MENU "Menu Options:" COLOR_RESET "\n");
        printf("  1. Add Line\n");
        printf("  2. Add Rectangle\n");
        printf("  3. Add Circle\n");
        printf("  4. Add Triangle\n");
        printf("  5. Delete Shape\n");
        printf("  6. Change Canvas Style (BG & Shape characters)\n");
        printf("  7. Clear All Shapes\n");
        printf("  8. Exit\n");
        int choice = read_int("\nSelect an option (1-8): ", 1, 8);
        if (choice == 8) {
            printf("\nExiting. Thank you for using the 2D Graphics Editor!\n");
            break;
        }
        if (choice >= 1 && choice <= 4) {
            if (state.object_count >= MAX_OBJECTS) {
                printf(COLOR_ERROR "Cannot add shape. Object limit (%d) reached!" COLOR_RESET "\n", MAX_OBJECTS);
                continue;
            }
            Shape shape;
            shape.draw_char = default_draw_char;
            if (choice == 1) {
                shape.type = SHAPE_LINE;
                printf("\n--- Add Line ---\n");
                shape.params.line.x1 = read_int("Enter X1 (0-79): ", 0, state.width - 1);
                shape.params.line.y1 = read_int("Enter Y1 (0-24): ", 0, state.height - 1);
                shape.params.line.x2 = read_int("Enter X2 (0-79): ", 0, state.width - 1);
                shape.params.line.y2 = read_int("Enter Y2 (0-24): ", 0, state.height - 1);
            } else if (choice == 2) {
                shape.type = SHAPE_RECTANGLE;
                printf("\n--- Add Rectangle ---\n");
                shape.params.rect.x = read_int("Enter top-left X (0-79): ", 0, state.width - 1);
                shape.params.rect.y = read_int("Enter top-left Y (0-24): ", 0, state.height - 1);
                int max_w = state.width - shape.params.rect.x;
                int max_h = state.height - shape.params.rect.y;
                shape.params.rect.width = read_int("Enter width (1-80): ", 1, max_w);
                shape.params.rect.height = read_int("Enter height (1-25): ", 1, max_h);
            } else if (choice == 3) {
                shape.type = SHAPE_CIRCLE;
                printf("\n--- Add Circle ---\n");
                shape.params.circle.cx = read_int("Enter center X (0-79): ", 0, state.width - 1);
                shape.params.circle.cy = read_int("Enter center Y (0-24): ", 0, state.height - 1);
                shape.params.circle.r = read_int("Enter radius (1-40): ", 1, 40);
            } else if (choice == 4) {
                shape.type = SHAPE_TRIANGLE;
                printf("\n--- Add Triangle ---\n");
                shape.params.triangle.x1 = read_int("Enter Vertex 1 X (0-79): ", 0, state.width - 1);
                shape.params.triangle.y1 = read_int("Enter Vertex 1 Y (0-24): ", 0, state.height - 1);
                shape.params.triangle.x2 = read_int("Enter Vertex 2 X (0-79): ", 0, state.width - 1);
                shape.params.triangle.y2 = read_int("Enter Vertex 2 Y (0-24): ", 0, state.height - 1);
                shape.params.triangle.x3 = read_int("Enter Vertex 3 X (0-79): ", 0, state.width - 1);
                shape.params.triangle.y3 = read_int("Enter Vertex 3 Y (0-24): ", 0, state.height - 1);
            }
            state.objects[state.object_count++] = shape;
            printf(COLOR_INFO "Shape added successfully!" COLOR_RESET "\n");
        } else if (choice == 5) {
            printf("\n--- Delete Shape ---\n");
            if (state.object_count == 0) {
                printf("No active shapes to delete.\n");
                continue;
            }
            int idx = read_int("Enter index of shape to delete: ", 0, state.object_count - 1);
            
            // Delete shape and compact array
            for (int i = idx; i < state.object_count - 1; i++) {
                state.objects[i] = state.objects[i + 1];
            }
            state.object_count--;
            printf(COLOR_INFO "Shape deleted successfully!" COLOR_RESET "\n");
        } else if (choice == 6) {
            printf("\n--- Change Canvas Style ---\n");
            char allowed_bg[] = "_ .#~- ";
            printf("Allowed background characters: '%s'\n", allowed_bg);
            state.bg_char = read_char("Enter new background character: ", allowed_bg);
            
            char allowed_draw[] = "*ox+#@$=_";
            printf("Allowed drawing characters: '%s'\n", allowed_draw);
            default_draw_char = read_char("Enter new drawing character: ", allowed_draw);
            
            // Update existing objects drawing character if preferred
            for (int i = 0; i < state.object_count; i++) {
                state.objects[i].draw_char = default_draw_char;
            }
            printf(COLOR_INFO "Styles updated!" COLOR_RESET "\n");
        } else if (choice == 7) {
            state.object_count = 0;
            printf(COLOR_INFO "Canvas cleared! All shapes deleted." COLOR_RESET "\n");
        }
        // Clear screen for redraw
        printf("\033[H\033[J");
    }
    return 0;
}
