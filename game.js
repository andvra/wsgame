var c = document.getElementById("myCanvas");
var ctx = c.getContext("2d");
var t_last = (new Date()).getTime();
var fps_update_t = 0.5;
var num_frames = 0;
var fps = 0;
var last_fps_update = (new Date()).getTime();
var bg = ctx.createImageData(c.width, c.height);
var d = bg.data;

var pt1 = {
    move_t: 0,
    start_x: 10,
    start_y: 10,
    cur_x: 10,
    cur_y: 10,
    target_x: 500,
    target_y: 500,
    max_speed: 1
};
var pt2 = {
    move_t: 0,
    start_x: 100,
    start_y: 100,
    cur_x: 100,
    cur_y: 100,
    target_x: 500,
    target_y: 400,
    max_speed: 2
};

for (var i = 0; i < c.width; i++) {
    for (var j = 0; j < c.height; j++) {
        d[4 * (i + c.width * j) + 0] = (i + 10 * j) % 256;
        d[4 * (i + c.width * j) + 1] = (5 * 5 + 2 * j) % 123;
        d[4 * (i + c.width * j) + 2] = 50 + (i + j) % 100;
        d[4 * (i + c.width * j) + 3] = 255;
    }
}

function parse_server_message(msg) {
    const json = JSON.parse(msg);
    const new_bezier_pts = json.bezier_pts;
    const new_player_pts = json.player_pts;
    bezier_pts = new_bezier_pts;
    player_pts = new_player_pts;
}

function update_pt(t_delta, pt) {
    const new_move_t = pt.move_t + t_delta
    const diff_x = pt.target_x - pt.cur_x;
    const diff_y = pt.target_y - pt.cur_y;
    const tot_diff = Math.sqrt(Math.pow(diff_x, 2) + Math.pow(diff_y, 2));
    var pt_new_x = 0;
    var pt_new_y = 0;
    if (tot_diff - pt.max_speed < 0.5) {
        pt_new_x = pt.target_x;
        pt_new_y = pt.target_y;
    } else {
        const acc = 0.01;
        const use_speed = Math.min(pt.max_speed, acc * new_move_t);
        const change_x = use_speed * diff_x / tot_diff;
        const change_y = use_speed * diff_y / tot_diff;
        pt_new_x = pt.cur_x + change_x;
        pt_new_y = pt.cur_y + change_y;
    }

    const ret = { ...pt, cur_x: pt_new_x, cur_y: pt_new_y, move_t: new_move_t };
    return ret;
}

function update_pts(t_delta) {
    pt1 = update_pt(t_delta, pt1);
    pt2 = update_pt(t_delta, pt2);
}

function factorial(v) {
    let ret = 1;

    for (let i = 2; i <= v; i++) {
        ret = ret * i;
    }

    return ret;
}

function binomial_coeff(n, k) {
    return factorial(n) / (factorial(k) * factorial(n - k));
}

function bezier(points, t) {
    var x = 0;
    var y = 0;

    let n = points.length - 1;
    for (let i = 0; i <= n; i++) {
        const binom = binomial_coeff(n, i);
        const f = binom * Math.pow((1 - t), n - i) * Math.pow(t, i);
        x += f * points[i].x;
        y += f * points[i].y;
    }

    return { x: x, y: y };
}

function normalize(x, y) {
    const d = Math.sqrt(Math.pow(x, 2) + Math.pow(y, 2));

    return { x: x / d, y: y / d };
}

let bezier_pts = [
    { x: 100, y: 300 },
    { x: 60, y: 100 },
    { x: 300, y: 100 },
    { x: 360, y: 300 },
    { x: 400, y: 140 }
];

let player_pts = [];

function draw() {
    var t_now = (new Date()).getTime();
    var t_delta = (t_now - t_last) / 1000; // Get in in seconds
    num_frames++;
    if (t_now - last_fps_update > fps_update_t) {

        fps = 1000 * num_frames / (t_now - last_fps_update);
        last_fps_update = t_now;
        num_frames = 0;
    }

    update_pts(t_delta);
    ctx.clearRect(0, 0, c.width, c.height);

    ctx.beginPath();
    ctx.putImageData(bg, 0, 0);
    ctx.font = "30px Arial";
    ctx.fillText("FPS: " + fps.toFixed(2), 10, 50);
    ctx.strokeStyle = "black"; // Green path
    ctx.moveTo(pt1.cur_x, pt1.cur_y);
    ctx.lineTo(pt2.cur_x, pt2.cur_y);
    ctx.stroke();

    ctx.strokeStyle = "black"; // Green path
    const num_curve_pts = 1000;
    const bezier_curve_segment_pts = [];
    const t_add = (new Date().getTime()) % 500 * 2 * Math.PI / 500.0;
    for (let i = 0; i < num_curve_pts; i++) {
        const bezier_val = bezier(bezier_pts, i / (num_curve_pts - 1));
        const x = bezier_val.x;
        const y = bezier_val.y;
        let x_add = 0;
        let y_add = 0;
        if (i > 0 && i < num_curve_pts - 1) {
            const amplitude_factor = 2;
            const freq = 5;
            const added_amplitude = amplitude_factor * Math.sin(t_add + freq * i * 2 * Math.PI / (num_curve_pts - 1));
            // Normal for 2d line (x1,y1), (x2,y2) ==>
            //  (y1-y2, x2-x1) and (y2-y1, x1-x2)
            const dx = x - bezier_curve_segment_pts[i - 1].base_x;
            const dy = y - bezier_curve_segment_pts[i - 1].base_y;
            const normal_start_pt = {
                x: -dy,
                y: dx
            };
            const normal = normal_start_pt;
            const normalized_normal = normalize(normal.x, normal.y);
            x_add = added_amplitude * normalized_normal.x;
            y_add = added_amplitude * normalized_normal.y;
        }

        bezier_curve_segment_pts.push({ base_x: x, base_y: y, cur_x: x + x_add, cur_y: y + y_add });

    }
    for (let i = 0; i < num_curve_pts - 1; i++) {
        ctx.beginPath();
        ctx.strokeStyle = "rgba(" + 255 * i / num_curve_pts + ",128,128)";
        ctx.lineWidth = 3;
        const start = bezier_curve_segment_pts[i];
        const end = bezier_curve_segment_pts[i + 1];
        ctx.moveTo(start.cur_x, start.cur_y);
        ctx.lineTo(end.cur_x, end.cur_y);
        ctx.stroke();
    }

    for (var i = 0; i < bezier_pts.length; i++) {
        ctx.beginPath();
        ctx.strokeStyle = "red";
        ctx.arc(bezier_pts[i].x, bezier_pts[i].y, 5, 0, 2 * Math.PI);
        ctx.stroke();
    }

    for (let i = 0; i < player_pts.length; i++) {
        ctx.beginPath();
        ctx.strokeStyle = "blue";
        ctx.arc(player_pts[i].x, player_pts[i].y, 5, 0, 2 * Math.PI);
        ctx.stroke();
    }

}
var ws;
function ws_init(ip_address) {
    const addr = "ws://" + ip_address + ":9002";
    ws = new WebSocket(addr);
    ws.onopen = function (e) {
        console.log('WebSocket Opened. Sending message..');
        ws_send_message("intro", { "msg": "Hi I'm a client" + Math.random() });
    };

    ws.onmessage = function (e) {
        parse_server_message(e.data);
    };

    ws.onclose = function (e) {
        console.log("Server shut down");
    }
}
function ws_send_message(msg_type, msg_keys) {
    var json = {
        "type": msg_type,
        ...msg_keys
    }
    ws.send(JSON.stringify(json));
}
function get_message() {
    var msg = document.getElementById('msg').value;
    if (!msg) {
        msg = "Default message from client";
    }
    return msg;
}

setInterval(draw, 10);
const log = document.getElementById('log');
var keys = [false, false, false, false];

function send_update_keys() {
    ws_send_message("keys", { "left": keys[0], "right": keys[1], "up": keys[2], "down": keys[3] })
}

function logkey(e, check_key_down) {
    const kc = e.keyCode;
    // TODO: We have the structure for key handling in place.
    //	Handle the different keys we are interested in,
    //	and send corresponding messages over the websocket connection
    const keycode_mapping = [
        { 'keycode': "A".charCodeAt(0), 'arr_index': 0 },
        { 'keycode': 68, 'arr_index': 1 },
        { 'keycode': 87, 'arr_index': 2 },
        { 'keycode': 83, 'arr_index': 3 },
    ]

    let interesting_update = false;
    for (const el of keycode_mapping) {
        if (kc == el.keycode && keys[el.arr_index] != check_key_down) {
            keys[el.arr_index] = check_key_down;
            interesting_update = true;
        }
    }

    if (interesting_update) {
        send_update_keys();
        console.log(keys);
        log.value = keys;
    }
}

function logKeyDown(e) {
    logkey(e, true);
}

function logKeyUp(e) {
    logkey(e, false);
}

function click_connect() {
    var ip = document.getElementById('ip').value;
    if (ip) {
        ws_init(ip);
    }
}

document.addEventListener('keydown', logKeyDown);
document.addEventListener('keyup', logKeyUp);
