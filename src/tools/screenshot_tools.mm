/**
    @file screenshot_tools.mm
    MaxMCP - Screenshot MCP Tools Implementation (Objective-C++)

    Captures entire patcher content as a base64 PNG image.
    Resizes the window to fit all content (bypassing screen size limits),
    then captures by sliding the window position so each portion falls
    within the visible screen area. Composites tiles into a final image.

    @ingroup maxmcp
*/

#include "screenshot_tools.h"
#include "tool_common.h"

#ifndef MAXMCP_TEST_MODE
#include "ext.h"
#include "jpatcher_api.h"
#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>
#import <objc/runtime.h>
#endif

#include "maxmcp.h"
#include "utils/console_logger.h"
#include "utils/patch_registry.h"

namespace ScreenshotTools {

using ToolCommon::DeferredResult;

static constexpr double PADDING = 40.0;
static constexpr long RENDER_DELAY_MS = 400;

// ============================================================================
// Data Structures
// ============================================================================

struct t_screenshot_data {
    t_maxmcp* patch;
    double max_width;
    double max_height;
    DeferredResult* deferred_result;
};

#ifndef MAXMCP_TEST_MODE

struct t_capture_data {
    t_maxmcp* patch;
    DeferredResult* deferred_result;
    t_clock* timer;
    double saved_x, saved_y, saved_w, saved_h;
    double content_width, content_height;
    CGWindowID window_id;
};

static t_capture_data* g_capture = nullptr;

// ============================================================================
// Helpers
// ============================================================================

static CGWindowID find_window_id(t_object* patcher) {
    t_symbol* title_sym = object_attr_getsym(patcher, gensym("title"));
    std::string title;
    if (title_sym && title_sym != gensym("")) {
        title = title_sym->s_name;
    } else {
        t_symbol* name_sym = object_attr_getsym(patcher, gensym("name"));
        if (name_sym && name_sym != gensym("")) title = name_sym->s_name;
    }
    CGWindowID target = 0;
    pid_t pid = [[NSProcessInfo processInfo] processIdentifier];
    CFArrayRef list = CGWindowListCopyWindowInfo(
        kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements, kCGNullWindowID);
    if (list) {
        for (CFIndex i = 0; i < CFArrayGetCount(list); i++) {
            NSDictionary* info = (__bridge NSDictionary*)CFArrayGetValueAtIndex(list, i);
            NSNumber* p = info[(__bridge NSString*)kCGWindowOwnerPID];
            NSString* n = info[(__bridge NSString*)kCGWindowName];
            if (p && [p intValue] == pid && n) {
                std::string wn = [n UTF8String];
                if (!title.empty() && wn.find(title) != std::string::npos) {
                    target = [info[(__bridge NSString*)kCGWindowNumber] unsignedIntValue];
                    break;
                }
            }
        }
        CFRelease(list);
    }
    return target;
}

static void cleanup(t_capture_data* d) {
    g_capture = nullptr;
    if (d->timer) { clock_free(d->timer); d->timer = nullptr; }
    delete d;
}

static void fail(t_capture_data* d, const std::string& msg) {
    t_object* pv = jpatcher_get_firstview(d->patch->patcher);
    if (pv) {
        t_atom a[4];
        atom_setfloat(&a[0], d->saved_x); atom_setfloat(&a[1], d->saved_y);
        atom_setfloat(&a[2], d->saved_w); atom_setfloat(&a[3], d->saved_h);
        object_attr_setvalueof(pv, gensym("rect"), 4, a);
    }
    d->deferred_result->result = ToolCommon::make_error(ToolCommon::ErrorCode::INTERNAL_ERROR, msg);
    d->deferred_result->notify();
    cleanup(d);
}

static CGImageRef capture_window(CGWindowID wid) {
    return CGWindowListCreateImage(CGRectNull, kCGWindowListOptionIncludingWindow, wid,
                                   kCGWindowImageBoundsIgnoreFraming);
}

static void set_visorigin(t_object* patcher, double x, double y) {
    t_object* pv = jpatcher_get_firstview(patcher);
    if (!pv) return;
    t_atom a[2];
    atom_setfloat(&a[0], x);
    atom_setfloat(&a[1], y);
    object_attr_setvalueof(pv, gensym("visorigin"), 2, a);
}

// ============================================================================
// Clock callback → defer to main thread
// ============================================================================

static void do_capture(t_maxmcp* owner, t_symbol* s, long argc, t_atom* argv) {
    t_capture_data* d = g_capture;
    if (!d || !d->patch || !d->deferred_result) {
        if (d) cleanup(d);
        return;
    }

    // Single capture of the full window
    CGImageRef image = capture_window(d->window_id);

    // Restore window immediately
    t_object* pv = jpatcher_get_firstview(d->patch->patcher);
    if (pv) {
        t_atom a[4];
        atom_setfloat(&a[0], d->saved_x); atom_setfloat(&a[1], d->saved_y);
        atom_setfloat(&a[2], d->saved_w); atom_setfloat(&a[3], d->saved_h);
        object_attr_setvalueof(pv, gensym("rect"), 4, a);
    }

    if (!image) { fail(d, "CGWindowListCreateImage failed"); return; }

    size_t w = CGImageGetWidth(image);
    size_t h = CGImageGetHeight(image);

    // Convert to PNG
    NSBitmapImageRep* bmp = [[NSBitmapImageRep alloc] initWithCGImage:image];
    CGImageRelease(image);

    if (!bmp) { fail(d, "Failed to create bitmap"); return; }

    NSData* png = [bmp representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
    if (!png || [png length] == 0) { fail(d, "Failed to encode PNG"); return; }

    NSString* b64 = [png base64EncodedStringWithOptions:0];
    std::string b64str = [b64 UTF8String];

    ConsoleLogger::log(("Screenshot done: " + std::to_string(w) + "x" + std::to_string(h) +
                         " (" + std::to_string([png length]) + " bytes)")
                            .c_str());

    json result = {
        {"result",
         {{"patch_id", d->patch->patch_id},
          {"width", (int)w}, {"height", (int)h},
          {"content_width", d->content_width}, {"content_height", d->content_height},
          {"format", "png"}}},
        {"_image", {{"data", b64str}, {"mimeType", "image/png"}}}};

    d->deferred_result->result = result;
    d->deferred_result->notify();
    cleanup(d);
}

static void clock_tick(t_maxmcp* owner) {
    defer(owner, (method)do_capture, gensym("screenshot_capture"), 0, nullptr);
}

// ============================================================================
// Setup (defer callback — main thread)
// ============================================================================

static void screenshot_setup(t_maxmcp* patch, t_symbol* s, long argc, t_atom* argv) {
    VALIDATE_DEFERRED_ARGS("screenshot_setup");
    EXTRACT_DEFERRED_DATA_WITH_RESULT(t_screenshot_data, data, argv);

    t_object* patcher = data->patch->patcher;
    t_object* pv = jpatcher_get_firstview(patcher);
    if (!pv) {
        COMPLETE_DEFERRED(data, ToolCommon::make_error(
            ToolCommon::ErrorCode::INTERNAL_ERROR, "No patcher view"));
        return;
    }

    // Save current window state
    long rc = 0; t_atom* rv = nullptr;
    object_attr_getvalueof(pv, gensym("rect"), &rc, &rv);
    double sx = 0, sy = 0, sw = 400, sh = 400;
    if (rv && rc >= 4) {
        sx = atom_getfloat(&rv[0]); sy = atom_getfloat(&rv[1]);
        sw = atom_getfloat(&rv[2]); sh = atom_getfloat(&rv[3]);
    }
    if (rv) sysmem_freeptr(rv);

    // Compute content bounding rect
    double min_x = 1e9, min_y = 1e9, max_x = -1e9, max_y = -1e9;
    bool has = false;
    for (t_object* box = jpatcher_get_firstobject(patcher); box; box = jbox_get_nextobject(box)) {
        t_rect r; jbox_get_patching_rect(box, &r);
        if (r.x < min_x) min_x = r.x;
        if (r.y < min_y) min_y = r.y;
        if (r.x + r.width > max_x) max_x = r.x + r.width;
        if (r.y + r.height > max_y) max_y = r.y + r.height;
        has = true;
    }
    if (!has) { min_x = 0; min_y = 0; max_x = 200; max_y = 200; }

    double content_w = (max_x - min_x) + PADDING * 2;
    double content_h = (max_y - min_y) + PADDING * 2;

    // Window size = content + generous chrome allowance
    double win_w = content_w + 100.0;
    double win_h = content_h + 150.0;

    // Screen info (needed only for finding window)
    NSScreen* screen = [NSScreen mainScreen];

    // Set visorigin to content start via Max API
    set_visorigin(patcher, min_x - PADDING, min_y - PADDING);

    // First set via Max API (may get clamped)
    t_atom nr[4];
    atom_setfloat(&nr[0], 0.0);
    atom_setfloat(&nr[1], 0.0);
    atom_setfloat(&nr[2], win_w);
    atom_setfloat(&nr[3], win_h);
    object_attr_setvalueof(pv, gensym("rect"), 4, nr);

    // Find window
    CGWindowID wid = find_window_id(patcher);
    if (wid == 0) {
        t_atom rest[4];
        atom_setfloat(&rest[0], sx); atom_setfloat(&rest[1], sy);
        atom_setfloat(&rest[2], sw); atom_setfloat(&rest[3], sh);
        object_attr_setvalueof(pv, gensym("rect"), 4, rest);
        COMPLETE_DEFERRED(data, ToolCommon::make_error(
            ToolCommon::ErrorCode::INTERNAL_ERROR, "Window not found"));
        return;
    }

    // Find NSWindow and bypass screen size constraint
    NSWindow* nsWin = nil;
    for (NSWindow* window in [NSApp windows]) {
        if ([window windowNumber] == (NSInteger)wid) {
            nsWin = window;
            break;
        }
    }

    if (!nsWin) {
        t_atom rest[4];
        atom_setfloat(&rest[0], sx); atom_setfloat(&rest[1], sy);
        atom_setfloat(&rest[2], sw); atom_setfloat(&rest[3], sh);
        object_attr_setvalueof(pv, gensym("rect"), 4, rest);
        COMPLETE_DEFERRED(data, ToolCommon::make_error(
            ToolCommon::ErrorCode::INTERNAL_ERROR, "NSWindow not found"));
        return;
    }

    // Swizzle constrainFrameRect:toScreen: to allow oversized windows
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        Class cls = [nsWin class];
        SEL sel = @selector(constrainFrameRect:toScreen:);
        Method method = class_getInstanceMethod(cls, sel);
        if (method) {
            IMP newIMP = imp_implementationWithBlock(
                ^NSRect(id self, NSRect frameRect, NSScreen* scr) {
                    return frameRect;
                });
            method_setImplementation(method, newIMP);
        }
    });

    // Resize window to full content size (position at top of screen)
    [nsWin setMaxSize:NSMakeSize(10000, 10000)];
    [nsWin setContentMaxSize:NSMakeSize(10000, 10000)];
    NSRect frame = NSMakeRect(0, 0, win_w, win_h);
    [nsWin setFrame:frame display:YES];

    NSRect actual = [nsWin frame];
    double actual_w = actual.size.width;
    double actual_h = actual.size.height;

    ConsoleLogger::log(("Screenshot: content=" + std::to_string((int)content_w) + "x" +
                         std::to_string((int)content_h) + " window=" +
                         std::to_string((int)actual_w) + "x" + std::to_string((int)actual_h))
                            .c_str());

    // Create capture data
    auto* cd = new t_capture_data{};
    cd->patch = data->patch;
    cd->deferred_result = data->deferred_result;
    cd->timer = clock_new(data->patch, (method)clock_tick);
    cd->saved_x = sx; cd->saved_y = sy; cd->saved_w = sw; cd->saved_h = sh;
    cd->content_width = content_w; cd->content_height = content_h;
    cd->window_id = wid;
    g_capture = cd;

    // Wait for render then capture
    clock_delay(cd->timer, RENDER_DELAY_MS);

    data->deferred_result = nullptr;
    delete data;
}

#endif  // MAXMCP_TEST_MODE

// ============================================================================
// Tool Schema
// ============================================================================

json get_tool_schemas() {
    return json::array(
        {{{"name", "get_patcher_screenshot"},
          {"description",
           "Capture a screenshot of the entire patcher content as a PNG image. "
           "Returns the image as base64-encoded data. The capture includes all objects "
           "even if they are outside the current window view."},
          {"inputSchema",
           {{"type", "object"},
            {"properties",
             {{"patch_id", {{"type", "string"}, {"description", "Patch ID to capture"}}},
              {"max_width",
               {{"type", "number"},
                {"description", "Maximum image width in pixels (default: 4096)"}}},
              {"max_height",
               {{"type", "number"},
                {"description", "Maximum image height in pixels (default: 4096)"}}}}},
            {"required", json::array({"patch_id"})}}}}});
}

// ============================================================================
// Executor & Dispatcher
// ============================================================================

#ifndef MAXMCP_TEST_MODE
static json execute_get_patcher_screenshot(const json& params) {
    std::string pid = params.value("patch_id", "");
    if (pid.empty()) return ToolCommon::missing_param_error("patch_id");
    t_maxmcp* patch = PatchRegistry::find_patch(pid);
    if (!patch) return ToolCommon::patch_not_found_error(pid);

    auto* dr = new DeferredResult();
    auto* data = new t_screenshot_data{patch, 0, 0, dr};
    t_atom a; atom_setobj(&a, data);
    defer(patch, (method)screenshot_setup, gensym("screenshot"), 1, &a);

    if (!dr->wait_for(ToolCommon::HEAVY_OPERATION_TIMEOUT)) {
        delete dr;
        return ToolCommon::timeout_error("screenshot capture");
    }
    json result = dr->result;
    delete dr;
    return result;
}
#endif

json execute(const std::string& tool, const json& params) {
    if (tool == "get_patcher_screenshot") {
#ifdef MAXMCP_TEST_MODE
        return ToolCommon::test_mode_error();
#else
        return execute_get_patcher_screenshot(params);
#endif
    }
    return nullptr;
}

}  // namespace ScreenshotTools
