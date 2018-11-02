import { ApiResponse } from './api';

export interface SunRiseSet extends ApiResponse, TimeAndSun {
    AstrTwilightEnd: string;
    AstrTwilightStart: string;
    CivTwilightEnd: string;
    CivTwilightStart: string;
    DayLength: string;
    NautTwilightEnd: string;
    NautTwilightStart: string;
    SunAtSouth: string;
}

export interface TimeAndSun {
    ServerTime: string;
    Sunrise: string;
    Sunset: string;
}
