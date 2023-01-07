load("render.star", "render")
load("http.star", "http")
load("time.star", "time")
load("humanize.star", "humanize")
load("schema.star", "schema")


BASE_URL = "https://vic.transportsg.me/mockups/metro-led-pids/"
DEFAULT_STATION = "flinders-street"
DEFAULT_PLATFORM = "*"

COLOR_MAP = {
    #Rail Lines
    "Sandringham": "#f178af",  
    "Frankston": "#028430", 
    "Werribee": "#028430",
    "Williamstoown": "#028430",
    "Cranbourne": "#279FD5",
    "Pakenham": "#279FD5",
    "Belgrave": "#152C6B",
    "Alamein": "#152C6B",
    "Glen Waverley": "#152C6B",
    "Sunbury": "#FFBE00",
    "Craigieburn": "#FFBE00",
    "Upfield": "#FFBE00",
    "Mernda": "#BE1014",
    "Hurstbridge": "#BE1014",
    "Showgrounds": "#95979A"

}



def main(config):
    platform = config.get("platform", DEFAULT_PLATFORM)
    station_name = config.get("station",DEFAULT_STATION)
    

    PTV_URL = BASE_URL + station_name + "/" + platform

    rep = http.post(PTV_URL, params={"csrf":"63b813903e397b35be2339e7"})
    if rep.status_code != 200:
        fail("No response from server, code %d", rep.status_code)
    data = rep.json()
    probably_not_working = False

    if len(data['dep']) > 0:
        departures_raw = data['dep']
    elif len(data['bus']) > 0:
        probably_not_working = True 
    elif data['has']:
        probably_not_working = True 
    else: # No trains at all
        probably_not_working = True


    if probably_not_working:
        train_number = render.Box(
            color = "#0000",
            width = 13,
            height = 8,
            child= render.Text("??", font = "CG-pixel-4x5-mono"),
        )

        return render.Root(
            render.Column(
                [
                    render.Row([
                        render.Stack(
                            [
                                render.Box(width=13,height=8,color=COLOR_MAP.get(station_name,"#95979A")),
                                train_number
                            ]
                        )
                        ,
                        render.Marquee(render.Text(station_name.replace("-"," ").title()),width=60),
                    ]),
                    render.Box(width=64, height = 1, color = "#666"),
                    render.Column([
                        render.Marquee(render.Text("Trains not running"), width=64)])

                ]
            )
        )



    else:

        one_train = departures_raw[0]

        child_train_number = render.Text(one_train['plt'], font = "CG-pixel-4x5-mono")

        train_number = render.Box(
            color = "#0000",
            width = 13,
            height = 8,
            child = child_train_number,
        )

        duration_to_departure = (time.parse_time(one_train['est'])-time.now()).minutes
        if duration_to_departure < 1:
            departure_text = "NOW"
        else:
            departure_text = str(int(duration_to_departure))
        to_depart = render.Box(
            color = "#0000",
            width = 13,
            height = 25,
            child = render.Column([
                render.WrappedText(departure_text,width=12,font="CG-pixel-3x5-mono",align='center')
            ])
        )

        return render.Root(
            render.Column(
                [
                    render.Row([
                        render.Stack(
                            [
                                render.Box(width=13,height=8,color=COLOR_MAP.get(one_train['route'],"#95979A")),
                                train_number
                            ]
                        )
                        ,
                        render.Marquee(render.Text(station_name.replace("-"," ").title()),width=60),
                    ]),
                    render.Box(width=64, height = 1, color = "#666"),
                    render.Row([
                        render.Stack([
                            render.Box(width=13,height=25,color="#0000ff"),
                            to_depart
                        ]),
                        render.Column([
                            render.Marquee(render.Text(one_train["dest"]), width=60),
                            render.Marquee(render.Text(one_train["txt"]), width=60),
                        ])
                    ])
                ]
            )
        )


def get_schema():
    return schema.Schema(
        version = "1",
        fields = [
            schema.Text(
                id = "station",
                name = "Train Station",
                desc = "Chosen station to view departures for",
                icon = "train",
            ),
            schema.Text(
                id = "platform",
                name = "Platform number",
                desc = "Platform to view departures on",
                icon = "user",
            ),
        ],
    )

"""
    if len(data['dep']) > 0:
        departures_raw = data['dep']
    elif len(data['bus']) > 0:
        replacement = True 
    elif data['has']:
        no_trains = True 
    else: # No trains at all
        no_depart = True

"""