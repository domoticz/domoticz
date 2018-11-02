export class ChartService {

  static setDeviceIdx(devIdx: string) {
    sessionStorage.setItem('chartDevIdx', devIdx);
  }

  static getDeviceIdx(): string {
    return sessionStorage.getItem('chartDevIdx');
  }

}
